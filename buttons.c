#include "stm32f0xx.h"
#include <stdint.h>
#include <stdlib.h>
#include "lcd.h"

static void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

void setup_buttons(void)
{
    // Only enable port C for the keypad
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOC->MODER &= ~0xffff;
    GPIOC->MODER |= 0x55 << (4*2);
    GPIOC->OTYPER &= ~0xff;
    GPIOC->OTYPER |= 0xf0;
    GPIOC->PUPDR &= ~0xff;
    GPIOC->PUPDR |= 0x55;
}

void drive_column(int c)
{
    GPIOC->BSRR = 0xf00000 | ~(1 << (c + 4));
}

int read_rows()
{
    return (~GPIOC->IDR) & 0xf;
}

int read_rows(void)
{
    for(;;)
        for(int c=0; c<4; c++) {
            drive_column(c);
            nano_wait(1000000); // wait 1 ms
            int r = read_rows();
            if (c==3) { // leftmost column
                if (r & 8) { // '1'
                    key = '1';
					break;
                }
                if (r & 4) { // '4'
                    key = '4';
					break;
                }
                if (r & 2) { // '7'
                    key = '7';
					break;
                }
            } else if (c == 2) { // column 2
                if (r & 8) { // '2'
                    key = '2';
					break;
                }
                if (r & 4) { // '5' re-center the ball
                    key = '5';
					break;
                }
                if (r & 2) { // '8'
                    key = '5';
					break;
                }
            } else if (c == 1) { // column 3
                if (r & 8) { // '3'
                    key = '3';
					break;
                }
                if (r & 4) { // '6'
                    key = '6';
					break;
                }
                if (r & 2) { // '9'
                    key = '9';
					break;
                }
            }
        }
}
