const int boardSize = 8;
const int numPieces = boardSize * boardSize / 2;

struct Coordinate {
  char x, y;
  Coordinate operator-(Coordinate& a) { return Coordinate{(char)(x-a.x),(char)(y-a.y)}; }
  bool operator==(Coordinate& a) { return x==a.x && y==a.y; }
  bool operator!=(Coordinate& a) { return !(*this==a); }
  bool isSquare() { return absoluteVal().x==absoluteVal().y; }
  Coordinate absoluteVal() { return Coordinate{(char)abs(x),(char)abs(y)}; }
};
enum PieceType { pawn, rook, knight, bishop, queen, king };
enum PieceColor { black, white };
struct Piece {
  Coordinate loc;
  PieceType type;
  PieceColor color;
  Piece() {}
  Piece(Coordinate loc,PieceType type,PieceColor color): loc(loc),type(type),color(color) {}
  bool alive = true;
  bool canMoveTo(Coordinate,Piece pieces[numPieces]);
};
struct Move {
  Coordinate start, finish;
  bool apply(Piece pieces[numPieces],PieceColor& whoseTurn); //returns valid or not
};

bool Piece::canMoveTo(Coordinate newLoc,Piece pieces[numPieces]) { //EN PASSANT? CASTLING?
  #define checkEveryPiece(cond) for (char i=0; i<numPieces; i++) { Piece& p = pieces[i]; if (&p!=this && p.alive && cond) return false; }
  #define inRange(a,r1,r2) ((r1>r2) ? (a<=r1&&a>=r2) : (a<=r2&&a>=r1))

  checkEveryPiece(p.color==color && p.loc==newLoc);
  Coordinate diff = newLoc-loc;
  Coordinate absDiff = diff.absoluteVal();
  if (type == knight) return (absDiff.x==2 && absDiff.y == 1) || (absDiff.x==1 && absDiff.y == 2);
  if (type == king) return absDiff.x <= 1 && absDiff.y <= 1;
  if (type == pawn) {
    if (color == white ? (loc.y != 1) : (loc.y != 6)) { if (color == white ? (diff.y != 1) : (diff.y != -1)) return false; } //move forward 1 if not in beginning row
    else if (color == white ? (diff.y != 1 && diff.y != 2) : (diff.y != -1 && diff.y != -2)) return false; //move forward 1 or 2 if in beginning row
    if (absDiff.y == 2 && absDiff.x == 1) return false;
    if (absDiff.y == 2) for (char i=0; i<numPieces; i++) if (pieces[i].loc.x == newLoc.x && pieces[i].loc.y == loc.y+(color == white ? 1 : -1)) return false;
    checkEveryPiece((p.loc == newLoc)&&(absDiff.x != 1 || absDiff.y != 1)); //if attacking move 1 diagonally
    return true;
  }

  if (type != bishop) //rook or queen
    if (absDiff.x == 0 || absDiff.y == 0) {
      checkEveryPiece(p.loc != newLoc && inRange(p.loc.x,loc.x,newLoc.x) && inRange(p.loc.y,loc.y,newLoc.y));
      return true;
    }
  if (type != rook) //bishop or queen
    if (diff.isSquare()) {
      checkEveryPiece(p.loc != newLoc && (p.loc-loc).isSquare() && inRange(p.loc.x,loc.x,newLoc.x) && inRange(p.loc.y,loc.y,newLoc.y));
      return true;
    }
  return false;
  
  #undef checkEveryPiece
  #undef inRange
}

bool Move::apply(Piece pieces[numPieces], PieceColor& whoseTurn) {
  for (char i=0; i<numPieces; i++) if (pieces[i].alive && pieces[i].loc==finish && pieces[i].color == whoseTurn) return false;
  for (char i=0; i<numPieces; i++) if (pieces[i].alive && pieces[i].loc==start) {
	if (pieces[i].color!=whoseTurn) return false;
	if (!pieces[i].canMoveTo(finish,pieces)) return false;
	for (char j=0; j<numPieces; j++) if (pieces[j].loc == finish) pieces[j].alive = false;
	pieces[i].loc = finish;
  	whoseTurn = whoseTurn==white ? black : white;
  	return true;
  }
  return false;
}

void populateBoard(Piece pieces[numPieces]) {
  PieceType backRow[boardSize] { rook, knight, bishop, queen, king, bishop, knight, rook };
  for (char i=0; i<boardSize; i++) {
    pieces[(4*i)  ]=Piece({i,0}, backRow[i], white);
    pieces[(4*i)+1]=Piece({i,1}, pawn,       white);
    pieces[(4*i)+2]=Piece({i,7}, backRow[i], black);
    pieces[(4*i)+3]=Piece({i,6}, pawn,       black);
  }
}

bool readInp(int pin) {
  for (int i=0; i<5; i++) {
    digitalWrite(9+i,pin%2);
    pin/=2;
  }
  delay(2);
  return digitalRead(A2);
}
void writeOtp(int pin, bool val) {
  for (int i=0; i<7; i++) {
    digitalWrite(2+i,pin%2);
    pin/=2;
  }
  digitalWrite(A1,val);
  delay(2);
  digitalWrite(A0,HIGH);
  delay(2);
  digitalWrite(A0,LOW);
}

Piece pieces[numPieces];
PieceColor whoseTurn = white;

const int receiveInPin = 0;
const int receiveOutPin = 0;
const int validInPin = 1;
const int validOutPin = 1;
const char amtDataPins = 3;
const int dataInPins[amtDataPins] = {2,3,4};
const int dataOutPins[amtDataPins] = {2,3,4};

char receiveData() {
  while (!readInp(receiveInPin));
  char data = 0;
  for (char i=0;i<amtDataPins;i++) { data <<= 1; data+=digitalRead(dataInPins[i]); }
  writeOtp(receiveOutPin,HIGH);
  while (readInp(receiveInPin));
  // must do digitalWrite(recieveOutPin,LOW) once you decide whether it's valid or not & output
  return data;
}
void sendValid(bool valid) {
  writeOtp(validOutPin,valid);
  delay(2);
  writeOtp(receiveOutPin,LOW);
}
char receiveDataAutoTrue() {
  char data = receiveData();
  sendValid(true);
  return data;
}
bool receiveMove(Piece pieces[numPieces], PieceColor& whoseTurn) { //returns whether it was valid
  char receivedData[4];
  for (char i=0;i<4;i++) { receivedData[i]=receiveData(); if (i!=3) sendValid(true); }
  Move m {{receiveDataAutoTrue(),receiveDataAutoTrue()},{receiveDataAutoTrue(),receiveData()}};
  bool valid = m.apply(pieces,whoseTurn);
  sendValid(valid);
  return valid;
}
bool sendData(char data) { //returns whether valid or not
  for (char i=0;i<amtDataPins;i++) writeOtp(dataOutPins[i],(data>>i)%2);
  delay(2);
  writeOtp(receiveOutPin,HIGH);
  while(!readInp(receiveInPin));
  writeOtp(receiveOutPin,LOW);
  while(readInp(receiveInPin));
  return readInp(validInPin);
}
bool sendMove(Move& m) {
  if (!sendData(m.start.x)) return false;
  if (!sendData(m.start.y)) return false;
  if (!sendData(m.finish.x)) return false;
  if (!sendData(m.finish.y)) return false;
  return true;
}

Move inputMove() {
  while (!readInp(5));
  char x1=0,y1=0,x2=0,y2=0;
  for (char i=0; i<3; i++) {
    if (readInp(6 +i)) x1 += 1 << i;
    if (readInp(9 +i)) y1 += 1 << i;
    if (readInp(12+i)) x2 += 1 << i;
    if (readInp(15+i)) y2 += 1 << i;
  }
  return Move{Coordinate{x1,y1},Coordinate{x2,y2}};
}

int lastBoard[numPieces] {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
void updateBoard(Piece pieces[numPieces]) {
  int board[numPieces];
  for (int p = 0; p<numPieces; p++) { //WRITE HIGH TO LOCS WITH PIECES ON THEM
    int loc = (pieces[p].color==white?0:1) + (pieces[p].loc.x*2) + (pieces[p].loc.y * 16) + /*plus starting pin num*/ dataOutPins[amtDataPins-1] + 1;
    if (loc>=37) loc += 64-37+loc;
    board[p] = loc;
    bool found = false;
    for (int p2 = 0; p2<numPieces; p2++) if (lastBoard[p2]==loc) { found = true; break; }
    if (!found) writeOtp(loc,HIGH);
  }
  for (int p = 0; p<numPieces; p++) { //WRITE LOW TO LOCS WITHOUT PIECES ON THEM
    bool found = false;
    int loc = lastBoard[p];
    for (int p2 = 0; p2<numPieces; p2++) if (board[p2]==loc) { found = true; break; }
    if (!found) writeOtp(loc,LOW);
  }
}

void setup() {
  populateBoard(pieces);
  Serial.begin(9600);
  for (Piece* piece=pieces;piece<pieces+numPieces;piece++) {
    Serial.print((int)piece->loc.x); Serial.print(','); Serial.println((int)piece->loc.y);
    Serial.println((int)piece->type);
    Serial.println((int)piece->color);
  }
  updateBoard(pieces);
}
void loop() {
  Move m;
  do m = inputMove();
  while (!sendMove(m));
  
  m.apply(pieces,whoseTurn);
  updateBoard(pieces);
  
  while (!receiveMove(pieces,whoseTurn));
  updateBoard(pieces);
}
