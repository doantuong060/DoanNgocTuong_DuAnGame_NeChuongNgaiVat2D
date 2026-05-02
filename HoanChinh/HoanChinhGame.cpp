/*
 * ============================================================
 *   NE CHUONG NGAI VAT 2D - GAME HOAN CHINH
 * ============================================================
 *  Compile:
 *    g++ main.cpp -o game.exe
 *        -lbgi -lgdi32 -lcomdlg32 -luuid -loleaut32 -lole32
 * ============================================================
 */

#include <winbgim.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <windows.h>

// ============================================================
//  HANG SO
// ============================================================
#define WIDTH           650
#define HEIGHT          750
#define MAX_OBS         15      // Tang len 15 vat the cung luc
#define MAX_ITEMS       4
#define SCREEN_MENU     0
#define SCREEN_HELP     1
#define SCREEN_GAME     2
#define ITEM_SHIELD     0
#define ITEM_HEART      1
#define SHIELD_DURATION 90

// Toc do roi vat the theo gameSpeed (tang dang ke)
// Speed 0=Dung, 1=Cham, 2=Nhanh, 3=Rat nhanh
const int FALL_SPEED[]   = { 0, 3, 7, 12 };

// Toc do di chuyen nhan vat theo gameSpeed
// Speed 0=Dung, 1=Binh thuong, 2=Nhanh hon, 3=Nhanh nhat
const int PLAYER_SPEED[] = { 0, 8, 12, 17 };

// Khoang cach spawn vat the (frame) theo gameSpeed
const int SPAWN_INTERVAL[] = { 999, 45, 28, 18 };

// ============================================================
//  CAU TRUC
// ============================================================
struct Obstacle { int x,y,type; bool active; };
struct Item     { int x,y,type; bool active; int animTimer; };

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

// --- Double buffer ---
int activePage=1, visualPage=0;

void flipBuffer()
{
    setvisualpage(activePage);
    activePage = 1 - activePage;
    setactivepage(activePage);
}

// ============================================================
//  [TP2] BRESENHAM DUONG THANG
// ============================================================
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

// ============================================================
//  [TP2] MIDPOINT DUONG TRON
// ============================================================
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

// ============================================================
//  [TP2] FLOOD FILL DE QUY
// ============================================================
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
//  [TP3] CAY FRACTAL
// ============================================================
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

// ============================================================
//  [TP3] KOCH SEGMENT
// ============================================================
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

void drawScenery()
{
    kochSegment(0,200,0,650,3); kochSegment(WIDTH,200,WIDTH,650,3);
    drawFractalTree(40,HEIGHT-90,90.0,35.0,6);
    drawFractalTree(WIDTH-40,HEIGHT-90,90.0,35.0,6);
}

// ============================================================
//  TRAI TIM UI
// ============================================================
void drawHeart(int x,int y)
{
    setcolor(4); setfillstyle(SOLID_FILL,4);
    fillellipse(x-5,y,5,5); fillellipse(x+5,y,5,5);
    int p[]={x-10,y,x+10,y,x,y+15,x-10,y}; fillpoly(4,p);
}
void drawHealth(){ int sx=WIDTH/3-70; for(int i=0;i<lives;i++) drawHeart(sx+i*25,60); }

// ============================================================
//  UI TREN CUNG
//  - Bo "Spd:X" o goc (da xoa theo yeu cau)
//  - Giu [SHIELD Xs] hien thi trong o Score
// ============================================================
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

    // Chi hien thi khien, KHONG hien thi Spd
    if(shieldActive){
        int rem=(SHIELD_DURATION-shieldTimer)/30+1;
        char sh[32]; sprintf(sh,"[SHIELD %ds]",rem);
        setcolor(11); settextstyle(DEFAULT_FONT,HORIZ_DIR,1);
        outtextxy(2*WIDTH/3+20,65,sh);
    }
}

// ============================================================
//  TEN GAME
// ============================================================
void drawTitle()
{
    settextstyle(TRIPLEX_FONT,HORIZ_DIR,3);
    char t[]="NE CHUONG NGAI VAT 2D"; int tw=textwidth(t);
    setcolor(8);  outtextxy(WIDTH/2-tw/2+3,113,t);
    setcolor(14); outtextxy(WIDTH/2-tw/2,  110,t);
}

// ============================================================
//  NHAN VAT
// ============================================================
void drawPlayer(int x,int y)
{
    if(shieldActive){ setcolor(11);bresenhamCircle(x,y,32); setcolor(9);bresenhamCircle(x,y,34); }
    setcolor(4); bresenhamCircle(x,y,18);
    setfillstyle(SOLID_FILL,4); fillellipse(x,y,18,24);
    setcolor(1); setfillstyle(SOLID_FILL,1);
    bar(x-14,y+12,x-4,y+26); bar(x+4,y+12,x+14,y+26);
    setcolor(8); setfillstyle(SOLID_FILL,8); bar(x+18,y-12,x+30,y+12);
    setcolor(9); setfillstyle(SOLID_FILL,9); fillellipse(x+5,y-5,11,7);
}

// ============================================================
//  CHUONG NGAI VAT
// ============================================================
void drawCylinder(int x,int y)
{ setcolor(11);bresenhamCircle(x,y,25);setfillstyle(SOLID_FILL,11);fillellipse(x,y,25,25); }

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

void drawObstacle(Obstacle& o)
{
    if(!o.active) return;
    switch(o.type){
        case 0:drawCylinder(o.x,o.y);break; case 1:drawTriangle(o.x,o.y);break;
        case 2:drawHalfCircle(o.x,o.y);break; case 3:drawSquare(o.x,o.y);break;
        case 4:drawDiamond(o.x,o.y);break;
    }
}

// ============================================================
//  VAT PHAM - vong tron nhap nhay
// ============================================================
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

// ============================================================
//  DIEU KHIEN PHIA DUOI
//  Bo dong chu "0=Dung 1=Cham..." theo yeu cau
// ============================================================
void drawControl()
{
    setcolor(8); bresenhamLine(0,HEIGHT-85,WIDTH,HEIGHT-85);
    int L[]={100,HEIGHT-50,150,HEIGHT-80,150,HEIGHT-60,240,HEIGHT-60,
             240,HEIGHT-40,150,HEIGHT-40,150,HEIGHT-20,100,HEIGHT-50};
    setcolor(10); setfillstyle(SOLID_FILL,10); fillpoly(8,L);
    int R[]={WIDTH-100,HEIGHT-50,WIDTH-150,HEIGHT-80,WIDTH-150,HEIGHT-60,
             WIDTH-240,HEIGHT-60,WIDTH-240,HEIGHT-40,WIDTH-150,HEIGHT-40,
             WIDTH-150,HEIGHT-20,WIDTH-100,HEIGHT-50};
    fillpoly(8,R);
    // Bo dong huong dan toc do theo yeu cau
}

bool checkCollision(int px,int py,int ox,int oy,int r)
{ int dx=px-ox,dy=py-oy; return dx*dx+dy*dy<r*r; }

// ============================================================
//  SPAWN
// ============================================================
void spawnObstacle()
{
    for(int i=0;i<MAX_OBS;i++)
        if(!obs[i].active){ obs[i].x=30+rand()%(WIDTH-60); obs[i].y=100; obs[i].type=rand()%5; obs[i].active=true; return; }
}

void spawnItem()
{
    // Giam xac suat xuong 5%
    if(rand()%100>=5) return;
    for(int i=0;i<MAX_ITEMS;i++)
        if(!items[i].active){ items[i].x=40+rand()%(WIDTH-80); items[i].y=100; items[i].type=rand()%2; items[i].active=true; items[i].animTimer=0; return; }
}

// ============================================================
//  KHOI TAO GAME
// ============================================================
void initGame()
{
    playerX=WIDTH/2; playerY=HEIGHT-130;
    lives=3; score=0; elapsedSec=0; gameSpeed=1;
    gameRunning=true; shieldActive=false; shieldTimer=0;
    for(int i=0;i<MAX_OBS;i++)   obs[i].active=false;
    for(int i=0;i<MAX_ITEMS;i++) items[i].active=false;
    // Spawn nhieu vat the ngay tu dau
    spawnObstacle(); spawnObstacle(); spawnObstacle();
}

// ============================================================
//  VE MENU - dung double buffer de khong nhay
// ============================================================
void drawMenuScreen()
{
    cleardevice();
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

    // Nut BAT DAU (xanh la) - thu nho, canh giua
    {
        int bw=160, bh=34, gap=18;
        int bx=WIDTH/2-bw/2;
        int by=370;
        setcolor(2); setfillstyle(SOLID_FILL,2);
        bar(bx,by,bx+bw,by+bh);
        settextstyle(DEFAULT_FONT,HORIZ_DIR,2);
        char s[]="BAT DAU"; int sw=textwidth(s);
        setcolor(15); outtextxy(bx+(bw-sw)/2, by+(bh-textheight(s))/2, s);
    }

    // Nut HUONG DAN (nau)
    {
        int bw=160, bh=34, gap=18;
        int bx=WIDTH/2-bw/2;
        int by=420;
        setcolor(6); setfillstyle(SOLID_FILL,6);
        bar(bx,by,bx+bw,by+bh);
        settextstyle(DEFAULT_FONT,HORIZ_DIR,2);
        char s[]="HUONG DAN"; int sw=textwidth(s);
        setcolor(15); outtextxy(bx+(bw-sw)/2, by+(bh-textheight(s))/2, s);
    }

    // Nut THOAT (do)
    {
        int bw=160, bh=34, gap=18;
        int bx=WIDTH/2-bw/2;
        int by=470;
        setcolor(4); setfillstyle(SOLID_FILL,4);
        bar(bx,by,bx+bw,by+bh);
        settextstyle(DEFAULT_FONT,HORIZ_DIR,2);
        char s[]="THOAT"; int sw=textwidth(s);
        setcolor(15); outtextxy(bx+(bw-sw)/2, by+(bh-textheight(s))/2, s);
    }

    settextstyle(DEFAULT_FONT,HORIZ_DIR,1); setcolor(8);
    {
        char ln1[]="Click nut hoac an phim tuong ung";
        char ln2[]="S = Choi  H = Huong dan  Q = Thoat";
        int w1=textwidth(ln1), w2=textwidth(ln2);
        outtextxy(WIDTH/2-w1/2, HEIGHT-55, ln1);
        outtextxy(WIDTH/2-w2/2, HEIGHT-38, ln2);
    }

    // Flip buffer de menu khong nhay
    flipBuffer();
}

// ============================================================
//  VE HUONG DAN
//  - Bo setbkcolor / to nen chu -> text hien thi sach
//  - Tang dy de cac dong khong bi de len nhau
// ============================================================
// scrollOffset cho man hinh huong dan
int helpScrollY = 0;
#define HELP_MAX_SCROLL 320   // pixels co the cuon xuong

void drawHelpScreen()
{
    cleardevice();

    // --- Tieu de co dinh ---
    setcolor(14); settextstyle(TRIPLEX_FONT,HORIZ_DIR,3);
    char htitle[]="HUONG DAN CHOI";
    int htw=textwidth(htitle);
    outtextxy(WIDTH/2-htw/2, 15, htitle);
    setcolor(8); bresenhamLine(10,62,WIDTH-16,62);

    // Viewport cho vung cuon (clip giua tieu de va footer)
    // Chieu cao viewport thuc su = HEIGHT-45-65 = HEIGHT-110
    int vpH = HEIGHT - 110;   // chieu cao vung cuon
    setviewport(0, 65, WIDTH-16, HEIGHT-45, 1);

    // y tinh trong he toa do viewport (0 = top cua viewport)
    int y  = 10 - helpScrollY;
    int dy = 22;
    int ITEM_H = 55;   // Chieu cao moi dong co icon (du cho hinh + chu + padding)

    settextstyle(DEFAULT_FONT,HORIZ_DIR,1);

    // ---- Phim dieu khien ----
    setcolor(10); outtextxy(30,y,"=== PHIM DIEU KHIEN ==="); y+=dy+4;
    setcolor(15);
    outtextxy(40,y,"LEFT / A   : Di chuyen trai");   y+=dy;
    outtextxy(40,y,"RIGHT / D  : Di chuyen phai");   y+=dy;
    outtextxy(40,y,"0          : Dung vat the");     y+=dy;
    outtextxy(40,y,"1          : Toc do Cham");      y+=dy;
    outtextxy(40,y,"2          : Toc do Nhanh");     y+=dy;
    outtextxy(40,y,"3          : Toc do Rat nhanh"); y+=dy;
    outtextxy(40,y,"ESC        : Ve menu");           y+=dy+10;

    // ---- Muc tieu ----
    setcolor(10); outtextxy(30,y,"=== MUC TIEU ==="); y+=dy+4;
    setcolor(15);
    outtextxy(40,y,"Ne tranh tat ca vat the roi xuong."); y+=dy;
    outtextxy(40,y,"Ne duoc 1 vat = +10 diem.");          y+=dy;
    outtextxy(40,y,"Co 3 mang. Het mang = thua.");        y+=dy+10;

    // ---- Vat pham ----
    setcolor(10); outtextxy(30,y,"=== VAT PHAM ==="); y+=dy+10;

    // KHIEN
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

    // TRAI TIM
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

    // ---- Chuong ngai vat ----
    setcolor(10); outtextxy(30,y,"=== CHUONG NGAI VAT ==="); y+=dy+10;

    // Cylinder
    cy = y + ITEM_H/2;
    if(cy > -30 && cy < vpH+30){ drawCylinder(55,cy); }
    setcolor(15); outtextxy(95,y+ITEM_H/2-8,": Hinh tron (Cylinder)");
    y += ITEM_H + 5;

    // Tam giac
    cy = y + ITEM_H/2;
    if(cy > -35 && cy < vpH+35){ drawTriangle(55,cy); }
    setcolor(15); outtextxy(95,y+ITEM_H/2-8,": Hinh tam giac");
    y += ITEM_H + 5;

    // Hinh vuong
    cy = y + ITEM_H/2;
    if(cy > -30 && cy < vpH+30){ drawSquare(55,cy); }
    setcolor(15); outtextxy(95,y+ITEM_H/2-8,": Hinh vuong");
    y += ITEM_H + 5;

    // Kim cuong
    cy = y + ITEM_H/2;
    if(cy > -35 && cy < vpH+35){ drawDiamond(55,cy); }
    setcolor(15); outtextxy(95,y+ITEM_H/2-8,": Kim cuong");
    y += ITEM_H + 5;

    // Nua duong tron
    cy = y + ITEM_H/2;
    if(cy > -30 && cy < vpH+30){ drawHalfCircle(55,cy); }
    setcolor(15); outtextxy(95,y+ITEM_H/2-8,": Nua duong tron");

    // Tat viewport
    setviewport(0, 0, WIDTH-1, HEIGHT-1, 1);

    // --- Thanh cuon ben phai ---
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

    // --- Footer: nut mui ten quay lai ---
    setcolor(8); bresenhamLine(10,HEIGHT-42,WIDTH-1,HEIGHT-42);

    // Nut mui ten quay lai: hinh tam giac + chu
    int arrowX=WIDTH/2, arrowY=HEIGHT-25;

    // Hinh mui ten trai (<)
    setcolor(10);
    bresenhamLine(arrowX-55, arrowY,   arrowX-40, arrowY-10);
    bresenhamLine(arrowX-55, arrowY,   arrowX-40, arrowY+10);
    bresenhamLine(arrowX-40, arrowY-10,arrowX-40, arrowY+10);
    setfillstyle(SOLID_FILL,10); floodfill(arrowX-45, arrowY, 10);

    // Chu "Quay lai menu"
    settextstyle(DEFAULT_FONT,HORIZ_DIR,1);
    char btxt[]="Quay lai menu";
    int btw=textwidth(btxt);
    setcolor(10);
    outtextxy(arrowX-30, arrowY-7, btxt);

    flipBuffer();
}

// ============================================================
//  GAME OVER
// ============================================================
void drawGameOverScreen()
{
    // Ve khung don gian, khong to nen xanh cuc lon
    setcolor(0); setfillstyle(SOLID_FILL,0);
    bar(WIDTH/2-200,HEIGHT/2-140,WIDTH/2+200,HEIGHT/2+140);

    // Vien khung
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
        // Khung: WIDTH/2-200 -> WIDTH/2+200, HEIGHT/2-140 -> HEIGHT/2+140
        // Canh 2 cum chu giua khung theo chieu ngang
        int boxCX  = WIDTH/2;
        int lineY  = HEIGHT/2 + 75;   // vi tri dong chu trong khung

        char r1[]="R: Choi lai";
        char r2[]="Q: Ve menu";
        int w1=textwidth(r1), w2=textwidth(r2);
        int gap=30;   // khoang cach giua 2 cum
        int totalW = w1+gap+w2;

        int x1 = boxCX - totalW/2;
        int x2 = x1 + w1 + gap;

        setcolor(10); outtextxy(x1, lineY, r1);
        setcolor(12); outtextxy(x2, lineY, r2);
    }
}

// ============================================================
//  VE FRAME GAME - DOUBLE BUFFERING
// ============================================================
void drawGameFrame()
{
    cleardevice();
    drawScenery();
    drawUI(); drawHealth(); drawTitle();
    drawPlayer(playerX,playerY);
    for(int i=0;i<MAX_OBS;i++)   drawObstacle(obs[i]);
    for(int i=0;i<MAX_ITEMS;i++) drawItem(items[i]);
    drawControl();
    if(!gameRunning) drawGameOverScreen();
    flipBuffer();
}

// ============================================================
//  CAP NHAT LOGIC
// ============================================================
void updateGame()
{
    if(!gameRunning) return;

    int fall = FALL_SPEED[gameSpeed];

    // Cap nhat khien
    if(shieldActive){
        shieldTimer++;
        if(shieldTimer>=SHIELD_DURATION){shieldActive=false;shieldTimer=0;}
    }

    static int spawnTimer=0;
    spawnTimer++;
    int si = SPAWN_INTERVAL[gameSpeed];
    if(spawnTimer>=si){
        // Spawn 2 vat the moi lan de man hinh day hon
        spawnObstacle();
        spawnObstacle();
        spawnItem();
        spawnTimer=0;
    }

    // Cap nhat chuong ngai vat
    for(int i=0;i<MAX_OBS;i++){
        if(!obs[i].active) continue;
        obs[i].y+=fall;
        if(obs[i].y>HEIGHT-85){
            obs[i].active=false;
            score+=10;
            if(score>highScore)highScore=score;
            continue;
        }
        if(checkCollision(playerX,playerY,obs[i].x,obs[i].y,40)){
            obs[i].active=false;
            if(!shieldActive){
                lives--;
                Beep(400,150);
                if(lives<=0){lives=0;gameRunning=false;}
            }
            else Beep(800,80);
        }
    }

    // Cap nhat vat pham
    for(int i=0;i<MAX_ITEMS;i++){
        if(!items[i].active) continue;
        items[i].y+=fall;
        if(items[i].y>HEIGHT-85){items[i].active=false;continue;}
        if(checkCollision(playerX,playerY,items[i].x,items[i].y,40)){
            items[i].active=false;
            if(items[i].type==ITEM_SHIELD){
                shieldActive=true; shieldTimer=0;
                Beep(600,100); Beep(900,100);
            }
            else{
                if(lives<3)lives++;
                Beep(700,80); Beep(1000,120);
            }
        }
    }
}

// ============================================================
//  INPUT MENU
// ============================================================
void handleMenuInput()
{
    if(GetAsyncKeyState('S')&0x8000){initGame();screenState=SCREEN_GAME;Sleep(200);return;}
    if(GetAsyncKeyState('H')&0x8000){screenState=SCREEN_HELP;Sleep(200);return;}
    if(GetAsyncKeyState('Q')&0x8000||GetAsyncKeyState(VK_ESCAPE)&0x8000){closegraph();exit(0);}
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

// ============================================================
//  INPUT GAME
//  - Toc do nhan vat tang theo gameSpeed
// ============================================================
void handleGameInput()
{
    int pspeed = PLAYER_SPEED[gameSpeed];

    if(GetAsyncKeyState(VK_LEFT)&0x8000||GetAsyncKeyState('A')&0x8000)
    { playerX-=pspeed; if(playerX<30)playerX=30; }

    if(GetAsyncKeyState(VK_RIGHT)&0x8000||GetAsyncKeyState('D')&0x8000)
    { playerX+=pspeed; if(playerX>WIDTH-30)playerX=WIDTH-30; }

    if(GetAsyncKeyState('0')&0x8000){gameSpeed=0;Sleep(100);}
    if(GetAsyncKeyState('1')&0x8000){gameSpeed=1;Sleep(100);}
    if(GetAsyncKeyState('2')&0x8000){gameSpeed=2;Sleep(100);}
    if(GetAsyncKeyState('3')&0x8000){gameSpeed=3;Sleep(100);}

    if(GetAsyncKeyState(VK_ESCAPE)&0x8000){screenState=SCREEN_MENU;Sleep(200);}

    if(!gameRunning){
        if(GetAsyncKeyState('R')&0x8000){initGame();screenState=SCREEN_GAME;Sleep(200);}
        if(GetAsyncKeyState('Q')&0x8000){screenState=SCREEN_MENU;Sleep(200);}
    }
}

// ============================================================
//  MAIN
// ============================================================
int main()
{
    srand((unsigned)time(NULL));
    initwindow(WIDTH,HEIGHT);

    // Khoi tao double buffering cho tat ca man hinh
    setactivepage(1);
    setvisualpage(0);
    activePage=1; visualPage=0;

    int frameCount=0, framesPerSec=30;

    while(true)
    {
        switch(screenState)
        {
            case SCREEN_MENU:
                // Menu cung dung double buffer -> khong nhay
                drawMenuScreen();       // da co flipBuffer() ben trong
                handleMenuInput();
                break;

            case SCREEN_HELP:
                // Xu ly cuon bang phim
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
                // Cuon bang PageUp/PageDown (thay the scroll chuot)
                if(GetAsyncKeyState(VK_PRIOR)&0x8000){   // PageUp = len
                    helpScrollY-=60;
                    if(helpScrollY<0)helpScrollY=0;
                    Sleep(80);
                }
                if(GetAsyncKeyState(VK_NEXT)&0x8000){    // PageDown = xuong
                    helpScrollY+=60;
                    if(helpScrollY>HELP_MAX_SCROLL)helpScrollY=HELP_MAX_SCROLL;
                    Sleep(80);
                }
                drawHelpScreen();

                // Chi thoat khi click vao nut "Quay lai menu" hoac ESC
                if(GetAsyncKeyState(VK_ESCAPE)&0x8000){
                    helpScrollY=0; screenState=SCREEN_MENU; Sleep(200);
                }
                if(ismouseclick(WM_LBUTTONDOWN)){
                    int hx=mousex(), hy=mousey();
                    clearmouseclick(WM_LBUTTONDOWN);
                    // Vung nut quay lai: mui ten + chu
                    if(hx>=WIDTH/2-60 && hx<=WIDTH/2+130 && hy>=HEIGHT-38 && hy<=HEIGHT-12){
                        helpScrollY=0;
                        screenState=SCREEN_MENU;
                    }
                }
                break;

            case SCREEN_GAME:
                handleGameInput();
                if(gameRunning){
                    frameCount++;
                    if(frameCount>=framesPerSec){elapsedSec++;frameCount=0;}
                    updateGame();
                }
                drawGameFrame();        // da co flipBuffer() ben trong
                break;
        }
        delay(33);
    }

    closegraph();
    return 0;
}
