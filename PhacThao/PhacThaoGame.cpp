#include <winbgim.h>
#include <conio.h>
#include <stdio.h>

#define WIDTH 650
#define HEIGHT 750

// ================== VE TRAI TIM ==================
void drawHeart(int x,int y)
{
    setcolor(4);
    setfillstyle(SOLID_FILL,4);

    fillellipse(x-5,y,5,5);
    fillellipse(x+5,y,5,5);

    int p[]={
        x-10,y,
        x+10,y,
        x,y+15,
        x-10,y
    };

    fillpoly(4,p);
}

// ================== THANH MAU ==================
void drawHealth()
{
    int startX = WIDTH/3 - 70;
    int y = 60;

    drawHeart(startX,y);
    drawHeart(startX+25,y);
    drawHeart(startX+50,y);
}

// ================== UI ==================
void drawUI()
{
    int topHeight = 75;
    int padding = 20;

    setcolor(2);
    rectangle(10,10,WIDTH-10,10+topHeight);

    line(WIDTH/3,10,WIDTH/3,10+topHeight);
    line(2*WIDTH/3,10,2*WIDTH/3,10+topHeight);

    setcolor(4);
    settextstyle(DEFAULT_FONT,HORIZ_DIR,2);

    outtextxy(10+padding,25,"Score:");
    outtextxy(WIDTH/3+padding,25,"Time:");
    outtextxy(2*WIDTH/3+padding,25,"High Score:");

    setcolor(15);
    outtextxy(10+padding,50,"0");
    outtextxy(WIDTH/3+padding,50,"00:00:00");
    outtextxy(2*WIDTH/3+padding,50,"0");
}

// ================== TEN GAME ==================
void drawTitle()
{
    settextstyle(TRIPLEX_FONT,HORIZ_DIR,3);

    char title[]="NE CHUONG NGAI VAT 2D";
    int textWidth=textwidth(title);

    setcolor(8);
    outtextxy(WIDTH/2-textWidth/2+3,113,title);

    setcolor(14);
    outtextxy(WIDTH/2-textWidth/2,110,title);
}

// ================== NUT HELP ==================
void drawHelpButton()
{
    int x = WIDTH - 35;
    int y = 60;

    setcolor(14);
    circle(x,y,15);

    settextstyle(DEFAULT_FONT,HORIZ_DIR,2);

    char q[]="?";

    int w=textwidth(q);
    int h=textheight(q);

    outtextxy(x-w/2,y-h/2,q);
}

// ================== NHAN VAT ==================
void drawPlayer(int x, int y)
{
    // ===== THAN =====
    setcolor(4);
    setfillstyle(SOLID_FILL,4);
    fillellipse(x, y, 18, 24);

    // ===== CHAN TRAI =====
    bar(x-14, y+12, x-4, y+26);

    // ===== CHAN PHAI =====
    bar(x+4, y+12, x+14, y+26);

    // ===== BA LO =====
    setcolor(8);
    setfillstyle(SOLID_FILL,8);
    bar(x+18, y-12, x+30, y+12);

    // ===== KINH =====
    setcolor(9);
    setfillstyle(SOLID_FILL,9);
    fillellipse(x+5, y-5, 11, 7);

    // ===== VIEN KINH =====
    setcolor(15);
    ellipse(x+5, y-5, 0, 360, 11, 7);
}

// ================== CHUONG NGAI VAT ==================
void drawCylinder(int x,int y)
{
    setcolor(11);
    setfillstyle(SOLID_FILL,11);

    circle(x,y,25);
    floodfill(x,y,11);
}

void drawTriangle(int x,int y)
{
    int p[]={
        x,y-30,
        x-25,y+20,
        x+25,y+20,
        x,y-30
    };

    setcolor(13);
    setfillstyle(SOLID_FILL,13);
    fillpoly(4,p);
}

void drawHalfCircle(int x,int y)
{
    setcolor(6);
    setfillstyle(SOLID_FILL,6);

    pieslice(x,y,0,180,25);
    
}

void drawSquare(int x,int y)
{
    setcolor(12);
    setfillstyle(SOLID_FILL,12);

    rectangle(x-25,y-25,x+25,y+25);
    floodfill(x,y,12);
}

// ================== DIEU KHIEN ==================
void drawControl()
{
    line(0,HEIGHT-85,WIDTH,HEIGHT-85);

    int leftArrow[]=
    {
        100,HEIGHT-50,
        150,HEIGHT-80,
        150,HEIGHT-60,
        240,HEIGHT-60,
        240,HEIGHT-40,
        150,HEIGHT-40,
        150,HEIGHT-20,
        100,HEIGHT-50
    };

    setcolor(10);
    setfillstyle(SOLID_FILL,10);
    fillpoly(8,leftArrow);

    int rightArrow[]=
    {
        WIDTH-100,HEIGHT-50,
        WIDTH-150,HEIGHT-80,
        WIDTH-150,HEIGHT-60,
        WIDTH-240,HEIGHT-60,
        WIDTH-240,HEIGHT-40,
        WIDTH-150,HEIGHT-40,
        WIDTH-150,HEIGHT-20,
        WIDTH-100,HEIGHT-50
    };

    fillpoly(8,rightArrow);
}

// ================== MAIN ==================
int main()
{
    initwindow(WIDTH,HEIGHT);
    setwindowtitle("Phac Thao Game Ne Chuong Ngai Vat");

    cleardevice();

    drawUI();
    drawHealth();
    drawTitle();
    drawHelpButton();

    drawPlayer(WIDTH/2, HEIGHT-130);

    int laneWidth = WIDTH/5;

    int lane1 = laneWidth/2;
    int lane2 = laneWidth + laneWidth/2;
    int lane3 = laneWidth*2 + laneWidth/2;
    int lane4 = laneWidth*3 + laneWidth/2;
    int lane5 = laneWidth*4 + laneWidth/2;

    drawCylinder(lane1,260);
    drawTriangle(lane2,300);
    drawHalfCircle(lane3,260);
    drawSquare(lane4,300);
    drawCylinder(lane5,280);

    drawControl();

    getch();
    closegraph();
}
