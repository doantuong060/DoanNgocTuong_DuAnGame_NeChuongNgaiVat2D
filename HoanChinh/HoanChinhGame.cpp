// Sinh vien: Ngo Thanh Nguyen
// MSSV: 2415053122330

#include <winbgim.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <windows.h>
#include <mmsystem.h>
// Compile: g++ main.cpp -o game.exe -lbgi -lgdi32 -lcomdlg32 -luuid -loleaut32 -lole32 -lwinmm

// ============================================================
//  HANG SO CO BAN
// ============================================================
#define WIDTH           650
#define HEIGHT          750
#define MAX_OBS         25  
#define MAX_ITEMS       4
#define SCREEN_MENU     0
#define SCREEN_HELP     1
#define SCREEN_GAME     2
#define ITEM_SHIELD     0
#define ITEM_HEART      1
#define SHIELD_DURATION 90 // Thoi gian ton tai cua khien (tinh bang frame)

// Toc do roi vat the theo gameSpeed (0=Dung, 1=Cham, 2=Nhanh, 3=Rat nhanh)
const int FALL_SPEED[]   = { 0, 3, 7, 12 };

// Toc do di chuyen nhan vat theo gameSpeed
const int PLAYER_SPEED[] = { 0, 8, 12, 17 };

// Khoang cach thoi gian tao vat the moi (spawn) tinh bang frame
const int SPAWN_INTERVAL[] = { 999, 40, 22, 12 };

// ============================================================
//  CAU TRUC DU LIEU
// ============================================================
// Chuong ngai vat
struct Obstacle {
    double x,y;         // Toa do X, Y (dung double de di chuyen muot hon)
    int    type;        // Loai hinh dang (0: tron, 1: tam giac,...)
    bool   active;      // Trang thai: true la dang roi tren man hinh
    double angle;       // Goc quay hien tai (do)
    double angleSpeed;  // Toc do xoay
    double scale;       // Ty le co gian (phong to/thu nho)
    double scaleSpeed;  // Toc do co gian
};

// Vat pham (mau, khien)
struct Item     { 
    int x,y,type; 
    bool active; 
    int animTimer;      // Bo dem de tao hieu ung nhap nhay
};

// ============================================================
//  BIEN TOAN CUC
// ============================================================
int  playerX, playerY;
int  lives=3, score=0, highScore=0, gameSpeed=1;
int  screenState=SCREEN_MENU, elapsedSec=0;
bool gameRunning=false, shieldActive=false;
int  shieldTimer=0;

Obstacle obs[MAX_OBS];
Item     items[MAX_ITEMS];

// Bien dung cho hieu ung dem nguoc truoc khi vao game
int countdownTimer = 0;
int countdownVal   = 3;

// Bien tao cam giac game (Game feel)
int    shakeTimer  = 0;        // Thoi gian rung man hinh khi va cham
int    shakeX=0,  shakeY=0;    // Toa do lech khi rung
int    flashTimer  = 0;        // Thoi gian chop do man hinh khi mat mang
double playerTilt  = 0.0;      // Goc nghieng cua nhan vat khi re trai/phai
bool   movingLeft  = false;
bool   movingRight = false;
int    globalFrame = 0;        

// Bien ho tro fix loi giu phim ESC (chong troi phim tu game ra menu)
bool   g_prevEsc   = false;

// --- Ky thuat Double Buffer (Chong giat lag man hinh) ---
int activePage=1, visualPage=0;

// Ham nay giup hoan doi trang dang ve va trang dang hien thi
void flipBuffer()
{
    setvisualpage(activePage);     // Hien thi trang vua ve xong
    activePage = 1 - activePage;   // Chuyen sang trang kia de ve tiep
    setactivepage(activePage);     // Dat trang do lam trang ve hien tai
}

// ============================================================
//  THUAT TOAN VE DO HOA CO BAN
// ============================================================
// Thuat toan Bresenham ve duong thang khong dung phep chia
void bresenhamLine(int x0,int y0,int x1,int y1)
{
    int dx=abs(x1-x0), dy=abs(y1-y0);
    int sx=(x0<x1)?1:-1, sy=(y0<y1)?1:-1, err=dx-dy;
    while(true){
        putpixel(x0,y0,getcolor());
        if(x0==x1&&y0==y1) break;
        int e2=2*err;
        if(e2>-dy){err-=dy;x0+=sx;}
        if(e2< dx){err+=dx;y0+=sy;}
    }
}

// Thuat toan Midpoint ve duong tron
void bresenhamCircle(int cx,int cy,int r)
{
    if(r<=0) return;
    int x=0,y=r,d=1-r;
    while(x<=y){
        putpixel(cx+x,cy+y,getcolor()); putpixel(cx-x,cy+y,getcolor());
        putpixel(cx+x,cy-y,getcolor()); putpixel(cx-x,cy-y,getcolor());
        putpixel(cx+y,cy+x,getcolor()); putpixel(cx-y,cy+x,getcolor());
        putpixel(cx+y,cy-x,getcolor()); putpixel(cx-y,cy-x,getcolor());
        x++;
        if(d<0) d+=2*x+1;
        else{y--;d+=2*(x-y)+1;}
    }
}

// Thuat toan to mau loang (Flood fill) de quy
void floodFillRecursive(int x,int y,int fill,int border)
{
    if(x<0||x>=WIDTH||y<0||y>=HEIGHT) return;
    int c=getpixel(x,y);
    if(c==fill||c==border) return;
    putpixel(x,y,fill);
    floodFillRecursive(x+1,y,fill,border);
    floodFillRecursive(x-1,y,fill,border);
    floodFillRecursive(x,y+1,fill,border);
    floodFillRecursive(x,y-1,fill,border);
}

// ============================================================
//  VE DO HOA FRACTAL
// ============================================================
// Thuat toan de quy ve cay Fractal (canh vat)
void drawFractalTree(int x1,int y1,double angle,double length,int depth)
{
    if(depth==0||length<2) return;
    int x2=(int)(x1+length*cos(angle*M_PI/180.0));
    int y2=(int)(y1-length*sin(angle*M_PI/180.0));
    setcolor(depth<=2?2:6);
    bresenhamLine(x1,y1,x2,y2);
    drawFractalTree(x2,y2,angle+25,length*0.70,depth-1);
    drawFractalTree(x2,y2,angle-25,length*0.70,depth-1);
}

// Thuat toan ve duong cong Koch (canh vat & mat dat)
void kochSegment(double x1,double y1,double x2,double y2,int depth)
{
    if(depth==0){setcolor(9);bresenhamLine((int)x1,(int)y1,(int)x2,(int)y2);return;}
    double xa=x1+(x2-x1)/3.0, ya=y1+(y2-y1)/3.0;
    double xb=x1+2.0*(x2-x1)/3.0, yb=y1+2.0*(y2-y1)/3.0;
    double ag=M_PI/3.0, dx=xb-xa, dy=yb-ya;
    double xp=xa+dx*cos(ag)-dy*sin(ag), yp=ya+dx*sin(ag)+dy*cos(ag);
    kochSegment(x1,y1,xa,ya,depth-1); kochSegment(xa,ya,xp,yp,depth-1);
    kochSegment(xp,yp,xb,yb,depth-1); kochSegment(xb,yb,x2,y2,depth-1);
}

// Cung Koch de tao hinh tron kieu Fractal (Su dung cho chuong ngai vat)
void drawKochArc(double x1,double y1,double x2,double y2, int depth, int col)
{
    if(depth==0){
        setcolor(col);
        bresenhamLine((int)x1,(int)y1,(int)x2,(int)y2);
        return;
    }
    double ax=x1+(x2-x1)/3.0, ay=y1+(y2-y1)/3.0;
    double bx=x1+2*(x2-x1)/3.0, by=y1+2*(y2-y1)/3.0;
    double ag=-M_PI/3.0;  
    double dx=bx-ax, dy=by-ay;
    double px=ax+dx*cos(ag)-dy*sin(ag);
    double py=ay+dx*sin(ag)+dy*cos(ag);
    drawKochArc(x1,y1,ax,ay,depth-1,col);
    drawKochArc(ax,ay,px,py,depth-1,col);
    drawKochArc(px,py,bx,by,depth-1,col);
    drawKochArc(bx,by,x2,y2,depth-1,col);
}

// Ham de quy ve duong cong Rong (Dragon Curve)
void drawDragonCurve(double x1, double y1, double x2, double y2, int depth, int sign, int color) 
{
    if (depth == 0) {
        setcolor(color);
        bresenhamLine((int)x1, (int)y1, (int)x2, (int)y2);
        return;
    }
    double dx = x2 - x1;
    double dy = y2 - y1;
    double x3 = x1 + (dx - sign * dy) / 2.0;
    double y3 = y1 + (dy + sign * dx) / 2.0;

    drawDragonCurve(x1, y1, x3, y3, depth - 1, 1, color);
    drawDragonCurve(x3, y3, x2, y2, depth - 1, -1, color);
}

// Ve toan bo background
void drawScenery()
{
    kochSegment(0,200,0,650,3); kochSegment(WIDTH,200,WIDTH,650,3);
    drawFractalTree(40,HEIGHT-90,90.0,35.0,6);
    drawFractalTree(WIDTH-40,HEIGHT-90,90.0,35.0,6);
}

// ============================================================
//  VE GIAO DIEN (UI)
// ============================================================
void drawHeart(int x,int y)
{
    setcolor(4); setfillstyle(SOLID_FILL,4);
    fillellipse(x-5,y,5,5); fillellipse(x+5,y,5,5);
    int p[]={x-10,y,x+10,y,x,y+15,x-10,y}; fillpoly(4,p);
}
void drawHealth(){ int sx=WIDTH/3-70; for(int i=0;i<lives;i++) drawHeart(sx+i*25,60); }

void drawUI()
{
    setcolor(2);
    bresenhamLine(10,10,WIDTH-10,10); bresenhamLine(10,10,10,85);
    bresenhamLine(10,85,WIDTH-10,85); bresenhamLine(WIDTH-10,10,WIDTH-10,85);
    bresenhamLine(WIDTH/3,10,WIDTH/3,85); bresenhamLine(2*WIDTH/3,10,2*WIDTH/3,85);

    setcolor(4); settextstyle(DEFAULT_FONT,HORIZ_DIR,2);
    outtextxy(30,25,"Score:"); outtextxy(WIDTH/3+20,25,"Time:"); outtextxy(2*WIDTH/3+20,25,"High Score:");

    char buf[64]; setcolor(15);
    sprintf(buf,"%d",score);                                 outtextxy(30,50,buf);
    sprintf(buf,"%02d:%02d",elapsedSec/60,elapsedSec%60);   outtextxy(WIDTH/3+20,50,buf);
    sprintf(buf,"%d",highScore);                             outtextxy(2*WIDTH/3+20,50,buf);

    if(shieldActive){
        int rem=(SHIELD_DURATION-shieldTimer)/30+1;
        char sh[32]; sprintf(sh,"[SHIELD %ds]",rem);
        setcolor(11); settextstyle(DEFAULT_FONT,HORIZ_DIR,1);
        outtextxy(2*WIDTH/3+20,65,sh);
    }
}

void drawTitle()
{
    settextstyle(TRIPLEX_FONT,HORIZ_DIR,3);
    char t[]="NE CHUONG NGAI VAT 2D"; int tw=textwidth(t);
    setcolor(8);  outtextxy(WIDTH/2-tw/2+3,113,t);
    setcolor(14); outtextxy(WIDTH/2-tw/2,  110,t);
}

// ============================================================
//  NHAN VAT & PHEP BIEN DOI DO HOA
// ============================================================
// Ham ho tro phep quay 2D quanh 1 tam cho truoc
void rotatePoint(double cx, double cy, double angle,
                 double px, double py,
                 int& outX, int& outY)
{
    double rad = angle * M_PI / 180.0;
    double dx  = px - cx;
    double dy  = py - cy;
    outX = (int)(cx + dx*cos(rad) - dy*sin(rad));
    outY = (int)(cy + dx*sin(rad) + dy*cos(rad));
}

// ============================================================
//  NHAN VAT: PHI HANH GIA (DETAILED RETRO ASTRONAUT)
// ============================================================
void drawPlayer(int x, int y) {
    if(shieldActive){
        setcolor(11); bresenhamCircle(x,y,38);
        setcolor(9);  bresenhamCircle(x,y,40);
    }

    // Tinh toan quan tinh mat kinh & balo khi di chuyen (Hieu ung 3D gia - Da tinh chinh)
    int kSide = movingLeft ? -7 : (movingRight ? 7 : 0);
    int bSide = movingLeft ? 14 : (movingRight ? -14 : 0);

    // Macro giup xoay bat ky toa do nao theo goc cua nhan vat
    #define ROT(px, py, ox, oy) rotatePoint(x, y, playerTilt, x + (px), y + (py), ox, oy)

    // Khai bao bien toa do tam thoi de hung toa do xoay
    int tx1, ty1, tx2, ty2, tx3, ty3, tx4, ty4, tx5, ty5, hx, hy;

    // --- 1. VE BALO NANG LUONG (JETPACK) - DESIGN M?I ---
    int bP[]={bSide - 15, -10, bSide + 15, -10, bSide + 15, 20, bSide - 15, 20, bSide - 15, -10};
    int bpR[10];
    for(int i=0; i<5; i++) ROT(bP[i*2], bP[i*2+1], bpR[i*2], bpR[i*2+1]);
    
    // Mau nen balo (Xam dam)
    setcolor(8); setfillstyle(SOLID_FILL, 8); fillpoly(5, bpR);
    // Cac loai nang luong (Polygon)
    setcolor(1); bresenhamLine(bpR[0],bpR[1], bpR[2],bpR[3]); // Vi?n trên

    // --- 2. VE THAN CHINH (BODY) & GIÁP ---
    int bo[]={-14, -8, 14, -8, 11, 22, -11, 22, -14, -8};
    int boR[10];
    for(int i=0; i<5; i++) ROT(bo[i*2], bo[i*2+1], boR[i*2], boR[i*2+1]);
    
    // Mau ao bao ho (Trang metallic)
    setcolor(15); setfillstyle(SOLID_FILL, 15); fillpoly(5, boR);

    // Giap nguc (Padding)
    setcolor(7);
    int pa1[]={-11, 8, 11, 8, 10, 16, -10, 16};
    int pa1R[8]; for(int i=0; i<4; i++) ROT(pa1[i*2], pa1[i*2+1], pa1R[i*2], pa1R[i*2+1]);
    setfillstyle(SOLID_FILL, 7); fillpoly(4, pa1R);

    // Soc trang tri mau do o that lung (Red Belt)
    int be[]={-11, 16, 11, 16, 11, 21, -11, 21};
    int beR[8]; for(int i=0; i<4; i++) ROT(be[i*2], be[i*2+1], beR[i*2], beR[i*2+1]);
    setcolor(4); setfillstyle(SOLID_FILL, 4); fillpoly(4, beR);

    // --- 3. VE CHAN (LEGS) & TAY (ARMS) ---
    // Mau Gray/Trang dan xen cho do bao ho
    setcolor(7); setfillstyle(SOLID_FILL, 7);

    // Chan
    int cL[]={-11, 22, -2, 22, -2, 36, -10, 36};
    int cLR[8]; for(int i=0; i<4; i++) ROT(cL[i*2], cL[i*2+1], cLR[i*2], cLR[i*2+1]);
    fillpoly(4, cLR);
    int cR[]={2, 22, 11, 22, 10, 36, 2, 36};
    int cRR[8]; for(int i=0; i<4; i++) ROT(cR[i*2], cR[i*2+1], cRR[i*2], cRR[i*2+1]);
    fillpoly(4, cRR);

    // Tay
    setcolor(15); setfillstyle(SOLID_FILL, 15);
    int tL[]={-18, -4, -13, -4, -13, 16, -18, 16};
    int tLR[8]; for(int i=0; i<4; i++) ROT(tL[i*2], tL[i*2+1], tLR[i*2], tLR[i*2+1]);
    fillpoly(4, tLR);
    int tR[]={13, -4, 18, -4, 18, 16, 13, 16};
    int tRR[8]; for(int i=0; i<4; i++) ROT(tR[i*2], tR[i*2+1], tRR[i*2], tRR[i*2+1]);
    fillpoly(4, tRR);

    // --- 4. VE MU BAO HIEM (HELMET) - DETAILED ---
    ROT(0, -18, hx, hy);
    setcolor(15); setfillstyle(SOLID_FILL, 15);
    fillellipse(hx, hy, 17, 17); // Helmet base

    // Ang-ten nh? ? M? (Detail)
    int ant1x, ant1y, ant2x, ant2y;
    ROT(-14, -26, ant1x, ant1y); ROT(-18, -32, ant2x, ant2y);
    setcolor(8); bresenhamLine(ant1x, ant1y, ant2x, ant2y);
    // Ðèn nháy trên ang-ten (Detail ð?ng)
    setcolor((globalFrame%20 < 10) ? 12 : 4); // Ch?p t?t mau ð?
    fillellipse(ant2x, ant2y, 2, 2);

    // --- 5. VE MAT KINH (VISOR) - NEON GRADIENT EFFECT ---
    // Visor mau Cyan dam (Mau nen)
    int vi[]={kSide - 11, -24, kSide + 11, -24, kSide + 9, -10, kSide - 9, -10};
    int viR[8]; for(int i=0; i<4; i++) ROT(vi[i*2], vi[i*2+1], viR[i*2], viR[i*2+1]);
    setcolor(1); setfillstyle(SOLID_FILL, 1); // Blue
    fillpoly(4, viR);

    // L?i visor ma Cyan (Neon)
    int vC[]={kSide - 9, -22, kSide + 9, -22, kSide + 7, -12, kSide - 7, -12};
    int vCR[8]; for(int i=0; i<4; i++) ROT(vC[i*2], vC[i*2+1], vCR[i*2], vCR[i*2+1]);
    setcolor(3); setfillstyle(SOLID_FILL, 3); // Cyan
    fillpoly(4, vCR);

    // Bong sang Visor (Highlight)
    ROT(kSide - 6, -20, hx, hy); ROT(kSide + 6, -20, hx, hy); // Diem tam thoi, su dung la duoc
    int gh1, gh2, gh3, gh4;
    ROT(kSide - 5, -20, gh1, gh2); ROT(kSide + 5, -20, gh3, gh4);
    setcolor(15); bresenhamLine(gh1, gh2, gh3, gh4); // Highlight trang

    #undef ROT // Xoa macro sau khi dung xong
}

// Cac ham ve hinh co ban khong dung phep bien doi
void drawTriangle(int x,int y)
{
    setcolor(13);
    bresenhamLine(x,y-30,x-25,y+20); bresenhamLine(x-25,y+20,x+25,y+20); bresenhamLine(x+25,y+20,x,y-30);
    int pts[]={x,y-30,x-25,y+20,x+25,y+20,x,y-30};
    setfillstyle(SOLID_FILL,13); fillpoly(4,pts);
}

void drawHalfCircle(int x,int y)
{ setcolor(6);setfillstyle(SOLID_FILL,6);pieslice(x,y,0,180,25);setcolor(14);bresenhamLine(x-25,y,x+25,y); }

void drawSquare(int x,int y)
{
    setcolor(12);
    bresenhamLine(x-25,y-25,x+25,y-25); bresenhamLine(x+25,y-25,x+25,y+25);
    bresenhamLine(x+25,y+25,x-25,y+25); bresenhamLine(x-25,y+25,x-25,y-25);
    setfillstyle(SOLID_FILL,12); floodfill(x,y,12);
}

void drawDiamond(int x,int y)
{
    setcolor(5);
    bresenhamLine(x,y-30,x+25,y); bresenhamLine(x+25,y,x,y+30);
    bresenhamLine(x,y+30,x-25,y); bresenhamLine(x-25,y,x,y-30);
    setfillstyle(SOLID_FILL,5); floodfill(x,y,5);
}

// ============================================================
//  VE VAT THE CO PHEP BIEN DOI (SCALE & ROTATE)
// ============================================================

//  Vat the: VONG TRON KOCH FRACTAL
void drawKochObstacleScaled(int cx, int cy, double angle, double sc)
{
    int r = (int)(26 * sc);
    setcolor(11); setfillstyle(SOLID_FILL,11);
    fillellipse(cx,cy,r,r);

    int sides = 6;
    for(int i=0;i<sides;i++){
        double a1 = angle*M_PI/180.0 + 2*M_PI*i/sides;
        double a2 = angle*M_PI/180.0 + 2*M_PI*(i+1)/sides;
        double x1 = cx + r * cos(a1);
        double y1 = cy + r * sin(a1);
        double x2 = cx + r * cos(a2);
        double y2 = cy + r * sin(a2);
        drawKochArc(x1,y1,x2,y2, 2, 15);  
    }
    setcolor(9); bresenhamCircle(cx,cy,r);
}

//  Vat the: NANG LUONG RONG FRACTAL
void drawDragonObstacleScaled(int x, int y, double angle, double sc) 
{
    double len = 40 * sc;
    int x1, y1, x2, y2;
    rotatePoint(x, y, angle, x - len, y, x1, y1);
    rotatePoint(x, y, angle, x + len, y, x2, y2);
    drawDragonCurve(x1, y1, x2, y2, 8, 1, 13);
    setcolor(5);
    bresenhamCircle(x, y, (int)(len * 0.3));
}

void drawHalfCircleScaled(int x, int y, double sc){
    int r = (int)(25 * sc);
    if(r < 5) r = 5;
    setcolor(6); setfillstyle(SOLID_FILL,6);
    pieslice(x,y,0,180,r);
    setcolor(14); bresenhamLine(x-r,y,x+r,y);
}

void drawTriangleScaled(int x, int y, double angle, double sc){
    double r = 30 * sc;
    int x1,y1,x2,y2,x3,y3;
    rotatePoint(x,y,angle, x,         y-r,    x1,y1);
    rotatePoint(x,y,angle, x-25*sc,   y+20*sc,x2,y2);
    rotatePoint(x,y,angle, x+25*sc,   y+20*sc,x3,y3);
    setcolor(13);
    bresenhamLine(x1,y1,x2,y2);
    bresenhamLine(x2,y2,x3,y3);
    bresenhamLine(x3,y3,x1,y1);
    int pts[]={x1,y1,x2,y2,x3,y3,x1,y1};
    setfillstyle(SOLID_FILL,13); fillpoly(4,pts);
}

void drawSquareScaled(int x, int y, double angle, double sc){
    double s = 22 * sc;
    int rx[4],ry[4];
    rotatePoint(x,y,angle, x-s,y-s, rx[0],ry[0]);
    rotatePoint(x,y,angle, x+s,y-s, rx[1],ry[1]);
    rotatePoint(x,y,angle, x+s,y+s, rx[2],ry[2]);
    rotatePoint(x,y,angle, x-s,y+s, rx[3],ry[3]);
    setcolor(12);
    bresenhamLine(rx[0],ry[0],rx[1],ry[1]);
    bresenhamLine(rx[1],ry[1],rx[2],ry[2]);
    bresenhamLine(rx[2],ry[2],rx[3],ry[3]);
    bresenhamLine(rx[3],ry[3],rx[0],ry[0]);
    int pts[]={rx[0],ry[0],rx[1],ry[1],rx[2],ry[2],rx[3],ry[3],rx[0],ry[0]};
    setfillstyle(SOLID_FILL,12); fillpoly(5,pts);
}

void drawDiamondScaled(int x, int y, double angle, double sc){
    double r = 30*sc, rh = 25*sc;
    int rx[4],ry[4];
    rotatePoint(x,y,angle, x,   y-r,  rx[0],ry[0]);
    rotatePoint(x,y,angle, x+rh,y,    rx[1],ry[1]);
    rotatePoint(x,y,angle, x,   y+r,  rx[2],ry[2]);
    rotatePoint(x,y,angle, x-rh,y,    rx[3],ry[3]);
    setcolor(5);
    bresenhamLine(rx[0],ry[0],rx[1],ry[1]);
    bresenhamLine(rx[1],ry[1],rx[2],ry[2]);
    bresenhamLine(rx[2],ry[2],rx[3],ry[3]);
    bresenhamLine(rx[3],ry[3],rx[0],ry[0]);
    int pts[]={rx[0],ry[0],rx[1],ry[1],rx[2],ry[2],rx[3],ry[3],rx[0],ry[0]};
    setfillstyle(SOLID_FILL,5); fillpoly(5,pts);
}

// Ham tong hop ve chuong ngai vat dua tren loai (type)
void drawObstacle(Obstacle& o)
{
    if(!o.active) return;
    double sc = o.scale;
    switch(o.type){
        case 0: drawKochObstacleScaled(o.x,o.y,o.angle,sc);  break; // Type 0 la Koch Curve
        case 2: drawHalfCircleScaled(o.x,o.y,sc);            break;
        case 1: drawTriangleScaled(o.x,o.y,o.angle,sc);      break;
        case 3: drawSquareScaled(o.x,o.y,o.angle,sc);        break;
        case 4: drawDiamondScaled(o.x,o.y,o.angle,sc);       break;
        case 5: drawDragonObstacleScaled(o.x,o.y,o.angle,sc);break; // Type 5 la Dragon Curve
    }
}

// Ham ve vat pham (mau hoac khien) voi hieu ung nhap nhay
void drawItem(Item& it)
{
    if(!it.active) return;
    it.animTimer++;
    int rim=(it.animTimer%16<8)?15:14;

    if(it.type==ITEM_SHIELD){
        setcolor(rim); bresenhamCircle(it.x,it.y,22);
        setfillstyle(SOLID_FILL,1); fillellipse(it.x,it.y,20,20);
        setcolor(11);
        bresenhamLine(it.x,   it.y-13,it.x+10,it.y-3);
        bresenhamLine(it.x+10,it.y-3, it.x,   it.y+13);
        bresenhamLine(it.x,   it.y+13,it.x-10,it.y-3);
        bresenhamLine(it.x-10,it.y-3, it.x,   it.y-13);
        setfillstyle(SOLID_FILL,9); floodfill(it.x,it.y,11);
        setcolor(15); bresenhamLine(it.x-9,it.y-2,it.x+9,it.y-2);
        settextstyle(DEFAULT_FONT,HORIZ_DIR,1); setcolor(14);
        outtextxy(it.x-4,it.y+3,"S");
    }
    else{
        setcolor(rim); bresenhamCircle(it.x,it.y,22);
        setfillstyle(SOLID_FILL,4); fillellipse(it.x,it.y,20,20);
        setcolor(15); setfillstyle(SOLID_FILL,12);
        fillellipse(it.x-5,it.y-3,5,5); fillellipse(it.x+5,it.y-3,5,5);
        int hp[]={it.x-10,it.y-3,it.x+10,it.y-3,it.x,it.y+9,it.x-10,it.y-3};
        fillpoly(4,hp);
        setcolor(15); settextstyle(DEFAULT_FONT,HORIZ_DIR,1);
        outtextxy(it.x-3,it.y+11,"+");
    }
}

// Ve nen dat phia duoi game
void drawKochGround()
{
    setcolor(3);
    kochSegment(0, HEIGHT-86, WIDTH/3, HEIGHT-86, 2);
    kochSegment(WIDTH/3, HEIGHT-86, 2*WIDTH/3, HEIGHT-86, 2);
    kochSegment(2*WIDTH/3, HEIGHT-86, WIDTH, HEIGHT-86, 2);
    setfillstyle(SOLID_FILL,1);
    bar(0, HEIGHT-84, WIDTH, HEIGHT);
}

void drawControl()
{
    drawKochGround();   
    int L[]={100,HEIGHT-50,150,HEIGHT-80,150,HEIGHT-60,240,HEIGHT-60,
             240,HEIGHT-40,150,HEIGHT-40,150,HEIGHT-20,100,HEIGHT-50};
    setcolor(10); setfillstyle(SOLID_FILL,10); fillpoly(8,L);
    int R[]={WIDTH-100,HEIGHT-50,WIDTH-150,HEIGHT-80,WIDTH-150,HEIGHT-60,
             WIDTH-240,HEIGHT-60,WIDTH-240,HEIGHT-40,WIDTH-150,HEIGHT-40,
             WIDTH-150,HEIGHT-20,WIDTH-100,HEIGHT-50};
    fillpoly(8,R);
}

// Ham kiem tra va cham hinh tron (khoang cach 2 diem < ban kinh)
bool checkCollision(int px,int py,int ox,int oy,int r)
{ int dx=px-ox,dy=py-oy; return dx*dx+dy*dy<r*r; }

// ============================================================
//  LOGIC SPAWN (TAO VAT THE)
// ============================================================
void spawnObstacle()
{
    for(int i=0;i<MAX_OBS;i++)
        if(!obs[i].active){
            obs[i].x          = 30 + rand()%(WIDTH-60);
            obs[i].y          = 100;
            // Co 6 loai tu 0 den 5 (bao gom ca Dragon)
            obs[i].type       = rand()%6;
            obs[i].active     = true;
            obs[i].angle      = (double)(rand()%360);
            // Cap nhat mang toc do xoay cho du 6 loai
            double rspd[]     = {3.0, 2.5, 0.0, 3.0, 2.0, -4.5}; 
            obs[i].angleSpeed = rspd[obs[i].type] * (rand()%2==0?1:-1);
            obs[i].scale      = 0.6 + (rand()%3)*0.1;   
            obs[i].scaleSpeed = 0.01 + (rand()%5)*0.002; 
            return;
        }
}

void spawnItem()
{
    // Ty le xuat hien la 5%
    if(rand()%100>=5) return;
    for(int i=0;i<MAX_ITEMS;i++)
        if(!items[i].active){ items[i].x=40+rand()%(WIDTH-80); items[i].y=100; items[i].type=rand()%2; items[i].active=true; items[i].animTimer=0; return; }
}

// ============================================================
//  AM THANH VA KHOI TAO
// ============================================================
// Ham lay duong dan den file am thanh
void getFullPath(const char* fname, char* out){
    GetModuleFileNameA(NULL,out,MAX_PATH);
    char* s=out;
    for(char* p=out;*p;p++) if(*p=='\\'||*p=='/') s=p+1;
    strcpy(s,fname);
}

// Nhac nen lap lai
void playBGM(){
    char path[MAX_PATH]; getFullPath("bgm.wav",path);
    PlaySoundA(path,NULL,SND_FILENAME|SND_ASYNC|SND_LOOP);
}
void stopBGM(){ PlaySoundA(NULL,NULL,0); }

void initGame()
{
    playerX=WIDTH/2; playerY=HEIGHT-130;
    lives=3; score=0; elapsedSec=0; gameSpeed=1;
    gameRunning=true; shieldActive=false; shieldTimer=0;
    for(int i=0;i<MAX_OBS;i++)    obs[i].active=false;
    for(int i=0;i<MAX_ITEMS;i++)  items[i].active=false;
    spawnObstacle(); spawnObstacle(); spawnObstacle();

    // Setup thoi gian dem nguoc
    countdownTimer = 90;   
    countdownVal   = 3;
    gameRunning    = false;   

    playBGM();
}

// ============================================================
//  CAC MAN HINH CUA GAME (SCREENS)
// ============================================================
void drawMenuScreen()
{
    setfillstyle(SOLID_FILL,0); setcolor(0);
    bar(0,0,WIDTH-1,HEIGHT-1);
    kochSegment(50,200,50,600,3); kochSegment(WIDTH-50,200,WIDTH-50,600,3);
    drawFractalTree(60,HEIGHT-20,90.0,40.0,5);
    drawFractalTree(WIDTH-60,HEIGHT-20,90.0,40.0,5);

    settextstyle(TRIPLEX_FONT,HORIZ_DIR,4);
    char t[]="NE CHUONG NGAI VAT"; int tw=textwidth(t);
    setcolor(8);  outtextxy(WIDTH/2-tw/2+4,124,t);
    setcolor(14); outtextxy(WIDTH/2-tw/2,  120,t);

    settextstyle(DEFAULT_FONT,HORIZ_DIR,2);
    char sub[]="2D ADVENTURE"; tw=textwidth(sub);
    setcolor(9); outtextxy(WIDTH/2-tw/2,195,sub);

    drawPlayer(WIDTH/2,310);

    {
        int bw=160, bh=34, gap=18;
        int bx=WIDTH/2-bw/2;
        int by=370;
        setcolor(2); setfillstyle(SOLID_FILL,2);
        bar(bx,by,bx+bw,by+bh);
        settextstyle(DEFAULT_FONT,HORIZ_DIR,2);
        char s[]="BAT DAU"; int sw=textwidth(s);
        setbkcolor(2); setcolor(15);
        outtextxy(bx+(bw-sw)/2, by+(bh-textheight(s))/2, s);
        setbkcolor(0);
    }
    {
        int bw=160, bh=34, gap=18;
        int bx=WIDTH/2-bw/2;
        int by=420;
        setcolor(6); setfillstyle(SOLID_FILL,6);
        bar(bx,by,bx+bw,by+bh);
        settextstyle(DEFAULT_FONT,HORIZ_DIR,2);
        char s[]="HUONG DAN"; int sw=textwidth(s);
        setbkcolor(6); setcolor(15);
        outtextxy(bx+(bw-sw)/2, by+(bh-textheight(s))/2, s);
        setbkcolor(0);
    }
    {
        int bw=160, bh=34, gap=18;
        int bx=WIDTH/2-bw/2;
        int by=470;
        setcolor(4); setfillstyle(SOLID_FILL,4);
        bar(bx,by,bx+bw,by+bh);
        settextstyle(DEFAULT_FONT,HORIZ_DIR,2);
        char s[]="THOAT"; int sw=textwidth(s);
        setbkcolor(4); setcolor(15);
        outtextxy(bx+(bw-sw)/2, by+(bh-textheight(s))/2, s);
        setbkcolor(0);
    }

    settextstyle(DEFAULT_FONT,HORIZ_DIR,1); setcolor(8);
    {
        char ln1[]="Click nut hoac an phim tuong ung";
        char ln2[]="S = Choi  H = Huong dan  Q = Thoat";
        int w1=textwidth(ln1), w2=textwidth(ln2);
        outtextxy(WIDTH/2-w1/2, HEIGHT-55, ln1);
        outtextxy(WIDTH/2-w2/2, HEIGHT-38, ln2);
    }

    flipBuffer();
}

int helpScrollY = 0;
// Tang Scroll limit de hien thi du ca 6 loai vat the
#define HELP_MAX_SCROLL 380   

void drawHelpScreen()
{
    setfillstyle(SOLID_FILL,0); setcolor(0);
    bar(0,0,WIDTH-1,HEIGHT-1);

    setcolor(14); settextstyle(TRIPLEX_FONT,HORIZ_DIR,3);
    char htitle[]="HUONG DAN CHOI";
    int htw=textwidth(htitle);
    outtextxy(WIDTH/2-htw/2, 15, htitle);
    setcolor(8); bresenhamLine(10,62,WIDTH-16,62);

    int vpH = HEIGHT - 110;   
    setviewport(0, 65, WIDTH-16, HEIGHT-45, 1); // Tao vung nhin (viewport) de cuon

    int y  = 10 - helpScrollY;
    int dy = 22;
    int ITEM_H = 55;   

    settextstyle(DEFAULT_FONT,HORIZ_DIR,1);

    setcolor(10); outtextxy(30,y,"=== PHIM DIEU KHIEN ==="); y+=dy+4;
    setcolor(15);
    outtextxy(40,y,"LEFT / A   : Di chuyen trai");   y+=dy;
    outtextxy(40,y,"RIGHT / D  : Di chuyen phai");   y+=dy;
    outtextxy(40,y,"0          : Dung vat the");     y+=dy;
    outtextxy(40,y,"1          : Toc do Cham");      y+=dy;
    outtextxy(40,y,"2          : Toc do Nhanh");     y+=dy;
    outtextxy(40,y,"3          : Toc do Rat nhanh"); y+=dy;
    outtextxy(40,y,"ESC        : Ve menu");           y+=dy+10;

    setcolor(10); outtextxy(30,y,"=== MUC TIEU ==="); y+=dy+4;
    setcolor(15);
    outtextxy(40,y,"Ne tranh tat ca vat the roi xuong."); y+=dy;
    outtextxy(40,y,"Ne duoc 1 vat = +10 diem.");          y+=dy;
    outtextxy(40,y,"Co 3 mang. Het mang = thua.");        y+=dy+10;

    setcolor(10); outtextxy(30,y,"=== VAT PHAM ==="); y+=dy+10;

    int cy = y + ITEM_H/2 - 5;
    if(cy > -20 && cy < vpH+20){
        setcolor(11); bresenhamCircle(55,cy,18);
        setfillstyle(SOLID_FILL,1); fillellipse(55,cy,16,16);
        setcolor(11);
        bresenhamLine(55,cy-12,63,cy-4); bresenhamLine(63,cy-4,55,cy+12);
        bresenhamLine(55,cy+12,47,cy-4); bresenhamLine(47,cy-4,55,cy-12);
        setfillstyle(SOLID_FILL,9); floodfill(55,cy,11);
        setcolor(15); bresenhamLine(47,cy-1,63,cy-1);
    }
    setcolor(11); outtextxy(90,y+ITEM_H/2-8,"KHIEN - Bao ve 3 giay");
    setcolor(15); outtextxy(90,y+ITEM_H/2+8,"(mien trung thuong)");
    y += ITEM_H + 5;

    cy = y + ITEM_H/2 - 5;
    if(cy > -20 && cy < vpH+20){
        setcolor(4); bresenhamCircle(55,cy,18);
        setfillstyle(SOLID_FILL,4); fillellipse(55,cy,16,16);
        setcolor(15); setfillstyle(SOLID_FILL,12);
        fillellipse(50,cy-3,5,5); fillellipse(60,cy-3,5,5);
        int hp[]={45,cy-3, 65,cy-3, 55,cy+9, 45,cy-3};
        fillpoly(4,hp);
    }
    setcolor(12); outtextxy(90,y+ITEM_H/2-8,"TRAI TIM - Hoi phuc");
    setcolor(15); outtextxy(90,y+ITEM_H/2+8,"1 mang (toi da 3)");
    y += ITEM_H + 10;

    setcolor(10); outtextxy(30,y,"=== CHUONG NGAI VAT ==="); y+=dy+10;

    // C?p nh?t ph?n Hi?n th? chý?ng ng?i v?t Fractal
    cy = y + ITEM_H/2;
    if(cy > -30 && cy < vpH+30){ drawKochObstacleScaled(55, cy, 15.0, 0.8); }
    setcolor(15); outtextxy(95,y+ITEM_H/2-8,": Vong tron Koch (Fractal)");
    y += ITEM_H + 5;

    cy = y + ITEM_H/2;
    if(cy > -35 && cy < vpH+35){ drawDragonObstacleScaled(55, cy, 45.0, 0.7); }
    setcolor(15); outtextxy(95,y+ITEM_H/2-8,": Loi nang luong Rong (Dragon)");
    y += ITEM_H + 5;

    cy = y + ITEM_H/2;
    if(cy > -35 && cy < vpH+35){ drawTriangle(55,cy); }
    setcolor(15); outtextxy(95,y+ITEM_H/2-8,": Hinh tam giac");
    y += ITEM_H + 5;

    cy = y + ITEM_H/2;
    if(cy > -30 && cy < vpH+30){ drawSquare(55,cy); }
    setcolor(15); outtextxy(95,y+ITEM_H/2-8,": Hinh vuong");
    y += ITEM_H + 5;

    cy = y + ITEM_H/2;
    if(cy > -35 && cy < vpH+35){ drawDiamond(55,cy); }
    setcolor(15); outtextxy(95,y+ITEM_H/2-8,": Kim cuong");
    y += ITEM_H + 5;

    cy = y + ITEM_H/2;
    if(cy > -30 && cy < vpH+30){ drawHalfCircle(55,cy); }
    setcolor(15); outtextxy(95,y+ITEM_H/2-8,": Nua duong tron");

    setviewport(0, 0, WIDTH-1, HEIGHT-1, 1);

    int trackTop=65, trackBot=HEIGHT-45, trackH=trackBot-trackTop;
    setcolor(0); setfillstyle(SOLID_FILL,0);
    bar(WIDTH-15,trackTop,WIDTH-1,trackBot);
    setcolor(3);
    bresenhamLine(WIDTH-15,trackTop,WIDTH-1,trackTop);
    bresenhamLine(WIDTH-1, trackTop,WIDTH-1,trackBot);
    bresenhamLine(WIDTH-1, trackBot,WIDTH-15,trackBot);
    bresenhamLine(WIDTH-15,trackBot,WIDTH-15,trackTop);
    int sbH=(int)(trackH*(float)vpH/(vpH+HELP_MAX_SCROLL));
    if(sbH<20)sbH=20;
    int sbY=trackTop+(int)((trackH-sbH)*(float)helpScrollY/HELP_MAX_SCROLL);
    setcolor(3); setfillstyle(SOLID_FILL,3);
    bar(WIDTH-14,sbY+1,WIDTH-2,sbY+sbH-1);

    setcolor(8); bresenhamLine(10,HEIGHT-42,WIDTH-1,HEIGHT-42);

    int arrowX=WIDTH/2, arrowY=HEIGHT-25;

    setcolor(10);
    bresenhamLine(arrowX-55, arrowY,   arrowX-40, arrowY-10);
    bresenhamLine(arrowX-55, arrowY,   arrowX-40, arrowY+10);
    bresenhamLine(arrowX-40, arrowY-10,arrowX-40, arrowY+10);
    setfillstyle(SOLID_FILL,10); floodfill(arrowX-45, arrowY, 10);

    settextstyle(DEFAULT_FONT,HORIZ_DIR,1);
    char btxt[]="Quay lai menu";
    int btw=textwidth(btxt);
    setcolor(10);
    outtextxy(arrowX-30, arrowY-7, btxt);

    flipBuffer();
}

void drawGameOverScreen()
{
    setcolor(0); setfillstyle(SOLID_FILL,0);
    bar(WIDTH/2-200,HEIGHT/2-140,WIDTH/2+200,HEIGHT/2+140);

    setcolor(12);
    bresenhamLine(WIDTH/2-200,HEIGHT/2-140,WIDTH/2+200,HEIGHT/2-140);
    bresenhamLine(WIDTH/2+200,HEIGHT/2-140,WIDTH/2+200,HEIGHT/2+140);
    bresenhamLine(WIDTH/2+200,HEIGHT/2+140,WIDTH/2-200,HEIGHT/2+140);
    bresenhamLine(WIDTH/2-200,HEIGHT/2+140,WIDTH/2-200,HEIGHT/2-140);

    setcolor(12); settextstyle(TRIPLEX_FONT,HORIZ_DIR,4);
    char go[]="GAME OVER"; int tw=textwidth(go);
    outtextxy(WIDTH/2-tw/2,HEIGHT/2-120,go);

    char buf[64]; settextstyle(DEFAULT_FONT,HORIZ_DIR,2); setcolor(15);
    sprintf(buf,"Diem so : %d",score);
    tw=textwidth(buf); outtextxy(WIDTH/2-tw/2,HEIGHT/2-30,buf);
    sprintf(buf,"Cao nhat: %d",highScore);
    tw=textwidth(buf); outtextxy(WIDTH/2-tw/2,HEIGHT/2+10,buf);

    settextstyle(DEFAULT_FONT,HORIZ_DIR,2);
    {
        int boxCX  = WIDTH/2;
        int lineY  = HEIGHT/2 + 75;   

        char r1[]="R: Choi lai";
        char r2[]="Q: Ve menu";
        int w1=textwidth(r1), w2=textwidth(r2);
        int gap=30;   
        int totalW = w1+gap+w2;

        int x1 = boxCX - totalW/2;
        int x2 = x1 + w1 + gap;

        setcolor(10); outtextxy(x1, lineY, r1);
        setcolor(12); outtextxy(x2, lineY, r2);
    }
}

// Ve toan bo khung hinh cua game moi nhip (frame)
void drawGameFrame()
{
    setfillstyle(SOLID_FILL,0); setcolor(0);
    bar(0,0,WIDTH-1,HEIGHT-1);

    // Hieu ung flash do
    if(flashTimer>0){
        setfillstyle(SOLID_FILL,4);
        bar(0,0,WIDTH,HEIGHT);
    }

    // Hieu ung rung man hinh
    setviewport(shakeX, shakeY, WIDTH-1+shakeX, HEIGHT-1+shakeY, 1);

    drawScenery();
    drawUI(); drawHealth(); drawTitle();
    drawPlayer(playerX,playerY);
    for(int i=0;i<MAX_OBS;i++)   drawObstacle(obs[i]);
    for(int i=0;i<MAX_ITEMS;i++) drawItem(items[i]);

    setviewport(0,0,WIDTH-1,HEIGHT-1,1);
    drawControl();

    // Ve text dem nguoc
    if(countdownTimer > 0){
        setcolor(0); setfillstyle(SOLID_FILL,0);
        bar(WIDTH/2-60, HEIGHT/2-60, WIDTH/2+60, HEIGHT/2+60);
        settextstyle(TRIPLEX_FONT,HORIZ_DIR,6);
        char cntStr[4]; sprintf(cntStr,"%d",countdownVal);
        int cw=textwidth(cntStr);
        int cntColors[]={12,14,10};
        int ci = countdownVal-1; if(ci<0)ci=0; if(ci>2)ci=2;
        setcolor(cntColors[ci]);
        outtextxy(WIDTH/2-cw/2, HEIGHT/2-40, cntStr);
    }
    else if(!gameRunning) drawGameOverScreen();
    flipBuffer();
}

// ============================================================
//  XU LY AM THANH HIEU UNG NHANH (SFX)
// ============================================================
// Cau truc va Thread de chay ham Beep() chay song song ma khong lam giat game
struct _BP { DWORD f,d; };
DWORD WINAPI _bTh(LPVOID p){
    _BP* b=(_BP*)p; Beep(b->f,b->d); delete b; return 0;
}
void _ba(DWORD hz,DWORD ms){
    _BP* b=new _BP; b->f=hz; b->d=ms;
    HANDLE h=CreateThread(NULL,0,_bTh,b,0,NULL);
    if(h) CloseHandle(h);
}

void sfxHit()  { _ba(300, 90);  }  // Tieng va cham
void sfxScore(){ _ba(880, 50);  }  // Tieng an diem
void sfxItem() { _ba(660, 70);  }  // Tieng an do

// Phat nhac thua
void sfxLose() { 
    char path[MAX_PATH]; getFullPath("lose.wav",path);
    PlaySoundA(path,NULL,SND_FILENAME|SND_ASYNC);
}
void beepAsync(int f,int d){ (void)f;(void)d; }

// ============================================================
//  HAM CAP NHAT LOGIC CUA GAME (MOI FRAME)
// ============================================================
void updateGame()
{
    // Xu ly logic dem nguoc truoc khi choi
    if(countdownTimer > 0){
        countdownTimer--;
        countdownVal = (countdownTimer / 30) + 1;
        if(countdownTimer == 0){
            gameRunning = true;   
        }
        return;
    }
    if(!gameRunning) return;

    globalFrame++;

    // Tinh toan rung man hinh
    if(shakeTimer>0){
        shakeTimer--;
        shakeX=(rand()%7)-3;
        shakeY=(rand()%5)-2;
    } else { shakeX=0; shakeY=0; }

    if(flashTimer>0) flashTimer--;

    // Tinh toan do nghieng nhan vat
    double targetTilt = movingLeft?-12.0:(movingRight?12.0:0.0);
    playerTilt += (targetTilt - playerTilt) * 0.22;

    int fall = FALL_SPEED[gameSpeed];

    // Giam thoi gian khien
    if(shieldActive){
        shieldTimer++;
        if(shieldTimer>=SHIELD_DURATION){shieldActive=false;shieldTimer=0;}
    }

    static int spawnTimer=0;
    spawnTimer++;
    int si = SPAWN_INTERVAL[gameSpeed];
    if(spawnTimer>=si){
        spawnObstacle();
        spawnObstacle();
        spawnItem();
        spawnTimer=0;
    }

    // Xu ly cap nhat tung chuong ngai vat
    for(int i=0;i<MAX_OBS;i++){
        if(!obs[i].active) continue;
        obs[i].y += fall;
        obs[i].angle += obs[i].angleSpeed;
        if(obs[i].angle >= 360.0) obs[i].angle -= 360.0;
        if(obs[i].angle <  0.0)   obs[i].angle += 360.0;
        
        // Them logic co gian vat the (Scale Up/Down) de kiem diem TP4
        obs[i].scale += obs[i].scaleSpeed;
        if(obs[i].scale > 1.3 || obs[i].scale < 0.6) obs[i].scaleSpeed *= -1; 

        // Neu vat the roi qua duoi man hinh
        if(obs[i].y > HEIGHT-85){
            obs[i].active = false;
            score += 10;
            if(score > highScore) highScore = score;
            sfxScore();   
            continue;
        }

        // Kiem tra va cham voi nhan vat
        if(checkCollision(playerX,playerY,obs[i].x,obs[i].y,40)){
            obs[i].active=false;
            if(!shieldActive){
                lives--;
                sfxHit();
                if(lives<=0){
                    lives=0; 
                    gameRunning=false; 
                    stopBGM(); 
                    sfxLose(); 
                }
            }
        }
    }

    // Xu ly vat pham (mau, khien)
    for(int i=0;i<MAX_ITEMS;i++){
        if(!items[i].active) continue;
        items[i].y+=fall;
        if(items[i].y>HEIGHT-85){items[i].active=false;continue;}

        // Kiem tra nhat do
        if(checkCollision(playerX,playerY,items[i].x,items[i].y,40)){
            items[i].active=false;
            if(items[i].type==ITEM_SHIELD){
                shieldActive=true; shieldTimer=0;
                sfxItem(); 
            }
            else{
                if(lives<3)lives++;
                sfxItem(); 
            }
        }
    }
}

// ============================================================
//  XU LY NHAP LIEU (INPUT)
// ============================================================
void handleMenuInput()
{
    if(GetAsyncKeyState('S')&0x8000){initGame();screenState=SCREEN_GAME;Sleep(200);return;}
    if(GetAsyncKeyState('H')&0x8000){screenState=SCREEN_HELP;Sleep(200);return;}
    if(GetAsyncKeyState('Q')&0x8000){stopBGM();closegraph();exit(0);}
    
    // Kiem tra nhan ESC va loai tru loi troi phim (g_prevEsc)
    if((GetAsyncKeyState(VK_ESCAPE)&0x8000) && !g_prevEsc){stopBGM();closegraph();exit(0);}
    
    if(ismouseclick(WM_LBUTTONDOWN)){
        clearmouseclick(WM_LBUTTONDOWN);
        int mx=mousex(),my=mousey();
        if(mx>=WIDTH/2-80&&mx<=WIDTH/2+80){
            if(my>=370&&my<=404){initGame();screenState=SCREEN_GAME;}
            if(my>=420&&my<=454){screenState=SCREEN_HELP;}
            if(my>=470&&my<=504){closegraph();exit(0);}
        }
    }
}

void handleGameInput()
{
    int pspeed = PLAYER_SPEED[gameSpeed];

    // Nhan giu phim de di chuyen
    bool lHeld=(GetAsyncKeyState(VK_LEFT)&0x8000)||(GetAsyncKeyState('A')&0x8000);
    bool rHeld=(GetAsyncKeyState(VK_RIGHT)&0x8000)||(GetAsyncKeyState('D')&0x8000);
    movingLeft=lHeld; movingRight=rHeld;
    if(lHeld){ playerX-=pspeed; if(playerX<30)    playerX=30; }
    if(rHeld){ playerX+=pspeed; if(playerX>WIDTH-30)playerX=WIDTH-30; }

    static bool k0=0,k1=0,k2=0,k3=0,kR=0,kQ=0;
    
    // Xu ly phim an mot lan (Edge detection) de doi toc do
    bool b0=(GetAsyncKeyState('0')&0x8000)!=0; if(b0&&!k0)gameSpeed=0; k0=b0;
    bool b1=(GetAsyncKeyState('1')&0x8000)!=0; if(b1&&!k1)gameSpeed=1; k1=b1;
    bool b2=(GetAsyncKeyState('2')&0x8000)!=0; if(b2&&!k2)gameSpeed=2; k2=b2;
    bool b3=(GetAsyncKeyState('3')&0x8000)!=0; if(b3&&!k3)gameSpeed=3; k3=b3;

    // Xu ly thoat ra menu khi nhan ESC
    bool bEsc=(GetAsyncKeyState(VK_ESCAPE)&0x8000)!=0;
    if(bEsc&&!g_prevEsc){stopBGM();screenState=SCREEN_MENU;} 

    if(!gameRunning){
        bool bR=(GetAsyncKeyState('R')&0x8000)!=0;
        if(bR&&!kR){initGame();screenState=SCREEN_GAME;} kR=bR;
        bool bQ=(GetAsyncKeyState('Q')&0x8000)!=0;
        if(bQ&&!kQ){stopBGM();screenState=SCREEN_MENU;} kQ=bQ;
    }
}

// ============================================================
//  HAM MAIN
// ============================================================
int main()
{
    srand((unsigned)time(NULL));
    initwindow(WIDTH,HEIGHT);

    // Thiet lap mac dinh cho double buffer
    setactivepage(1);
    setvisualpage(0);
    activePage=1; visualPage=0;

    int frameCount=0, framesPerSec=30;

    // Vong lap game chinh (Main Game Loop)
    while(true)
    {
        switch(screenState)
        {
            case SCREEN_MENU:
                drawMenuScreen();       
                handleMenuInput();
                break;

            case SCREEN_HELP:
                // Tinh toan thanh cuon khi doc huong dan
                if(GetAsyncKeyState(VK_DOWN)&0x8000){
                    helpScrollY+=18;
                    if(helpScrollY>HELP_MAX_SCROLL)helpScrollY=HELP_MAX_SCROLL;
                    Sleep(60);
                }
                if(GetAsyncKeyState(VK_UP)&0x8000){
                    helpScrollY-=18;
                    if(helpScrollY<0)helpScrollY=0;
                    Sleep(60);
                }
                if(GetAsyncKeyState(VK_PRIOR)&0x8000){   
                    helpScrollY-=60;
                    if(helpScrollY<0)helpScrollY=0;
                    Sleep(80);
                }
                if(GetAsyncKeyState(VK_NEXT)&0x8000){    
                    helpScrollY+=60;
                    if(helpScrollY>HELP_MAX_SCROLL)helpScrollY=HELP_MAX_SCROLL;
                    Sleep(80);
                }
                drawHelpScreen();

                {
                    bool bEsc = (GetAsyncKeyState(VK_ESCAPE)&0x8000)!=0;
                    if(bEsc && !g_prevEsc){
                        helpScrollY=0; screenState=SCREEN_MENU;
                    }
                }

                if(ismouseclick(WM_LBUTTONDOWN)){
                    int hx=mousex(), hy=mousey();
                    clearmouseclick(WM_LBUTTONDOWN);
                    if(hx>=WIDTH/2-60 && hx<=WIDTH/2+130 && hy>=HEIGHT-38 && hy<=HEIGHT-12){
                        helpScrollY=0;
                        screenState=SCREEN_MENU;
                    }
                }
                break;

            case SCREEN_GAME:
                handleGameInput();
                if(gameRunning || countdownTimer > 0){
                    if(gameRunning){   
                        frameCount++;
                        if(frameCount>=framesPerSec){elapsedSec++;frameCount=0;}
                    }
                    updateGame();
                }
                drawGameFrame();
                break;
        }

        // Cap nhat trang thai phim ESC cuoi moi frame de su dung cho frame tiep theo (Fix bug thoat)
        g_prevEsc = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
        
        delay(33); // Tao thoi gian nghi 33ms (~30 FPS)
    }

    closegraph();
    return 0;
}
