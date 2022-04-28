#include "stm32f0xx.h"
#include <stdint.h>
#include <stdlib.h>
#include "lcd.h"

#define UP    '2'
#define DOWN  '8'
#define LEFT  '4'
#define RIGHT '6'


static void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

void setup_tim17()
{
    RCC->APB2ENR |= RCC_APB2ENR_TIM17EN;
    TIM17->PSC = 480-1;
    TIM17->ARR = 100-1;
    TIM17->DIER |= TIM_DIER_UIE;
    TIM17->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] = 0x00400000;
}

void setup_tim16()
{
    RCC->APB2ENR |= RCC_APB2ENR_TIM16EN;
    TIM16->PSC = 4800-1;
    TIM16->ARR = 1000-1;
    TIM16->DIER |= TIM_DIER_UIE;
    TIM16->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] = 0x00200000;
}

void drive_column(int c)
{
    GPIOC->BSRR = 0xf00000 | ~(1 << (c + 4));
}

int read_rows()
{
    return (~GPIOC->IDR) & 0xf;
}

char check_key()
{
    //return the key if pressed
}

// Copy a subset of a large source picture into a smaller destination.
// sx,sy are the offset into the source picture.
void pic_subset(Picture *dst, const Picture *src, int sx, int sy)
{
    int dw = dst->width;
    int dh = dst->height;
    for(int y=0; y<dh; y++) {
        if (y+sy < 0)
            continue;
        if (y+sy >= src->height)
            break;
        for(int x=0; x<dw; x++) {
            if (x+sx < 0)
                continue;
            if (x+sx >= src->width)
                break;
            dst->pix2[dw * y + x] = src->pix2[src->width * (y+sy) + x + sx];
        }
    }
}

// Overlay a picture onto a destination picture.
// xoffset,yoffset are the offset into the destination picture that the
// source picture is placed.
// Any pixel in the source that is the 'transparent' color will not be
// copied.  This defines a color in the source that can be used as a
// transparent color.
void pic_overlay(Picture *dst, int xoffset, int yoffset, const Picture *src, int transparent)
{
    for(int y=0; y<src->height; y++) {
        int dy = y+yoffset;
        if (dy < 0)
            continue;
        if (dy >= dst->height)
            break;
        for(int x=0; x<src->width; x++) {
            int dx = x+xoffset;
            if (dx < 0)
                continue;
            if (dx >= dst->width)
                break;
            unsigned short int p = src->pix2[y*src->width + x];
            if (p != transparent)
                dst->pix2[dy*dst->width + dx] = p;
        }
    }
}

extern const Picture background; // A 240x320 background image
extern const Picture spaceship; // A 19x19 purple ball with white boundaries

//Border of the LCD
int xmin;
int xmax;
int ymin;
int ymax;
int x, y; //Center of the spaceship
int life; //Life of the game
int level; //Level will increase as game goes on
int time; //Timer for increasing the difficulty

// This C macro will create an array of Picture elements.
// Really, you'll just use it as a pointer to a single Picture
// element with an internal pix2[] array large enough to hold
// an image of the specified size.
// BE CAREFUL HOW LARGE OF A PICTURE YOU TRY TO CREATE:
// A 100x100 picture uses 20000 bytes.  You have 32768 bytes of SRAM.
#define TempPicturePtr(name,width,height) Picture name[(width)*(height)/6+2] = { {width,height,2} }
TempPicturePtr(object,29,29);
TempPicturePtr(scroll,240,10);
TempPicturePtr(crashed,20,20);
TempPicturePtr(bullet,10,10);

void erase(int x, int y)
{
    TempPicturePtr(tmp,29,29); // Create a temporary 29x29 image.
    pic_subset(tmp, &background, x-tmp->width/2, y-tmp->height/2); // Copy the background
    //pic_overlay(tmp, 5,5, &ball, 0xffff); // Overlay the ball
    LCD_DrawPicture(x-tmp->width/2,y-tmp->height/2, tmp); // Draw
}

void update(int x, int y)
{
    LCD_DrawPicture(x-spaceship.width/2,y-spaceship.height/2, &spaceship); // Draw the ball
}

void update2(int x, int y)
{
    TempPicturePtr(tmp,29,29); // Create a temporary 29x29 image.
    pic_subset(tmp, &background, x-tmp->width/2, y-tmp->height/2); // Copy the background
    pic_overlay(tmp, 5,5, &spaceship, 0xffff); // Overlay the ball
    LCD_DrawPicture(x-tmp->width/2,y-tmp->height/2, tmp); // Draw
}

void TIM17_IRQHandler()
{
    TIM17->SR &= ~TIM_SR_UIF;
    char key = check_key();
    if (key == UP && y > ymin) {
        y -= 1;
    } else if (key == DOWN && y < ymax) {
        y += 1;
    } else if (key == RIGHT && x < xmax) {
        x += 1;
    } else if (key == LEFT && x > xmin) {
        x -= 1;
    }

    pic_subset(crashed, &background, x-spaceship.width/2, y-spaceship.height/2);
    for (int i = 0; i < 20*20; i++) {
        if (crashed->pix2[i] == 0xFFFF) {
            life--;
            // Play the crash music here
            
            if (life == 0) {
                LCD_DrawFillRectangle(0,0,240,320,BLACK);
                LCD_DrawString(20,141, WHITE, BLACK, "GAME OVER!", 16, 0);
                NVIC->ICER[0] = 1<<TIM16_IRQn;
                NVIC->ICER[0] = 1<<TIM17_IRQn;
            }
            break;
        }
    }

    TempPicturePtr(tmp, 29, 29);
    pic_subset(tmp, &background, x-tmp->width/2, y-tmp->height/2);
    pic_overlay(tmp,0,0, object, 0xffff);
    LCD_DrawPicture(x-tmp->width/2, y-tmp->width/2,tmp);
    

}

void TIM16IRQ_Handler()
{
    TIM16->SR &= ~TIM_SR_UIF;
    for (int i = 30; i > 0; i--) {
        pic_subset(scroll, &background, 0, i * 10);
        pic_overlay(scroll, 0, i*10 +10, background);
        LCD_DrawPicture(0, i*10 + 10, scroll);
    }
    
    pic_subset(scroll, &background, 0, 0);
    for (int z = 0; z < 4 + (level - 1) * 2; z++) {
        int r = random() % 23;
        pic_overlay(bullet, r * 10, 0, scroll);
    }
    
    LCD_DrawPicture(x-spaceship.width/2, y-spaceship.height/2,spaceship);
    time += 1;
    if (time % 10 == 0) {
        level++;
    }
}

void game_setup(void)
{
    // Draw the background.
    LCD_DrawPicture(0,0,&background);
    LCD_DrawString(20,141, WHITE, BLACK, "Bullet Hell", 16, 0);
    LCD_DrawString(20,191, WHITE, BLACK, "Press D to start", 16, 0);
}

void game_start(void)
{
    while(check_key() != 'D');
    xmin = spaceship.width/2;
    xmax = background.width - spaceship.width/2;
    ymin = spaceship.width/2;
    ymax = background.height - spaceship.height/2;

    for(int i=0; i<29*29; i++) {
        object->pix2[i] = 0xffff;
    }

    x = (xmin+xmax) / 2;
    y = ymin;
    life = 3;
    level = 1;
    time = 0;

    setup_tim16();
    setup_tim17();
}

void basic_drawing(void)
{
    LCD_Clear(0);
    LCD_DrawRectangle(10, 10, 30, 50, GREEN);
    LCD_DrawFillRectangle(50, 10, 70, 50, BLUE);
    LCD_DrawLine(10, 10, 70, 50, RED);
    LCD_Circle(50, 90, 40, 1, CYAN);
    LCD_DrawTriangle(90,10, 120,10, 90,30, YELLOW);
    LCD_DrawFillTriangle(90,90, 120,120, 90,120, GRAY);
    LCD_DrawFillRectangle(10, 140, 120, 159, WHITE);
    LCD_DrawString(20,141, BLACK, WHITE, "Test string!", 16, 0); // opaque background
}
