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
    else return color == white ? (diff.y != 1 && diff.y != 2) : (diff.y != -1 && diff.y != -2); //move forward 1 or 2 if in beginning row
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
  bool valid = false;
  for (char i=0; i<numPieces; i++) if (pieces[i].alive && pieces[i].loc==start) valid = pieces[i].color==whoseTurn && pieces[i].canMoveTo(finish,pieces);
  if (!valid) return false;
  Piece pieces2[numPieces];
  memcpy(pieces2,pieces,numPieces);
  //find if that puts king in check
  Coordinate kingLoc;
  for (char i=0; i<numPieces; i++) if (pieces2[i].color==whoseTurn && pieces2[i].type==king) { kingLoc = pieces2[i].loc; break; }
  for (char i=0; i<numPieces; i++) if (pieces2[i].alive && pieces2[i].color!=whoseTurn && pieces2[i].canMoveTo(kingLoc,pieces2)) return false;
  memcpy(pieces,pieces2,numPieces);
  whoseTurn = whoseTurn==white ? black : white;
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

Piece pieces[numPieces];

const int receiveInPin = 0;
const int receiveOutPin = 0;
const int validInPin = 0;
const int validOutPin = 0;
const char amtDataPins = 3;
const int dataInPins[amtDataPins] = {0,0,0};
const int dataOutPins[amtDataPins] = {0,0,0};

char receiveData() {
  while (!digitalRead(receiveInPin));
  char data = 0;
  for (char i=0;i<amtDataPins;i++) { data <<= 1; data+=digitalRead(dataInPins[i]); }
  digitalWrite(receiveOutPin,HIGH);
  while (digitalRead(receiveInPin));
  // must do digitalWrite(recieveOutPin,LOW) once you decide whether it's valid or not & output
  return data;
}
bool sendData(char data) { //returns whether valid or not
  for (char i=0;i<amtDataPins;i++) digitalWrite(dataOutPins[i],(data>>i)%2);
  digitalWrite(receiveOutPin,HIGH);
  while(!digitalRead(receiveInPin));
  digitalWrite(receiveOutPin,LOW);
  while(digitalRead(receiveInPin));
  return digitalRead(validInPin);
}

void setup() {
  populateBoard(pieces);
}
void loop() {}
