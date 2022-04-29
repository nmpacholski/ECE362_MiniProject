#include "stm32f0xx.h"
#include "lcd.h"

void init_lcd_spi(void)
{
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	GPIOB->MODER &= ~(GPIO_MODER_MODER3 | GPIO_MODER_MODER5 |GPIO_MODER_MODER8 | GPIO_MODER_MODER11 | GPIO_MODER_MODER14);
	GPIOB->MODER |= GPIO_MODER_MODER3_1 | GPIO_MODER_MODER5_1 | GPIO_MODER_MODER8_0 | GPIO_MODER_MODER11_0 | GPIO_MODER_MODER14_0;
	GPIOB->ODR |= GPIO_ODR_8 | GPIO_ODR_11 | GPIO_ODR_14;
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
	SPI1->CR1 &= ~ SPI_CR1_SPE;
	SPI1->CR1 &= ~ SPI_CR1_BR;
	SPI1->CR1 |= SPI_CR1_MSTR;
	SPI1->CR2 = 0; //sets ds to 0000, which equals to default of 8
	SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;
	SPI1->CR1 |= SPI_CR1_SPE;
}

void setup_buttons(void)
{
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	GPIOC->MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1 | GPIO_MODER_MODER2 | GPIO_MODER_MODER3 | GPIO_MODER_MODER4 | GPIO_MODER_MODER5 | GPIO_MODER_MODER6 | GPIO_MODER_MODER7);
	GPIOC->MODER |= GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0 | GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0;
	GPIOC->OTYPER |= GPIO_OTYPER_OT_4 | GPIO_OTYPER_OT_5 | GPIO_OTYPER_OT_6 | GPIO_OTYPER_OT_7;
	GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPDR0 | GPIO_PUPDR_PUPDR1 | GPIO_PUPDR_PUPDR2 | GPIO_PUPDR_PUPDR3);
	GPIOC->PUPDR |= GPIO_PUPDR_PUPDR0_0 | GPIO_PUPDR_PUPDR1_0 | GPIO_PUPDR_PUPDR2_0 | GPIO_PUPDR_PUPDR3_0;
}

void basic_drawing(void);
void move_ball(void);

int main(void)
{
    setup_buttons();
    LCD_Setup(); // this will call init_lcd_spi()
    basic_drawing();
    //move_ball();
}