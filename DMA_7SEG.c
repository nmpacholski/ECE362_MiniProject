#include "stm32f0xx.h"
#include "lcd.h"

void set_char_msg(int, char);
void nano_wait(unsigned int);

int msg_index = 0;
uint16_t msg[8] = { 0x0000,0x0100,0x0200,0x0300,0x0400,0x0500,0x0600,0x0700 };
extern const char font[];

void init_spi2(void)
{
    // setup GPIOB pins
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~0xcf000000; // clear pins
    GPIOB->MODER |= 0x8a000000; // PB12,13,15 AF(10)
    GPIOB->AFR[1] &= ~0xf0000; // PB12 = SPI2_NSS
    GPIOB->AFR[1] &= ~0xf00000; // PB13 = SPI2_SCK
    GPIOB->AFR[1] &= ~0xf0000000; // PB15 = SPI2_MOSI
    // setup SPI2
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN; // RCC for SPI2
    SPI2->CR1 &= ~SPI_CR1_SPE; // disable SPI2
    SPI2->CR1 |= SPI_CR1_BR; // clk/256
    SPI2->CR1 |= SPI_CR1_MSTR; // master mode
    SPI2->CR2 = SPI_CR2_TXDMAEN | SPI_CR2_NSSP | SPI_CR2_DS; // SPI2->CR2
    SPI2->CR1 |= SPI_CR1_SPE; // enable SPI2
}

void setup_dma(void)
{
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel5->CCR &= ~DMA_CCR_EN; // turn off channel
    DMA1_Channel5->CPAR = (uint32_t) (&(SPI2->DR)); // GPIOB_ODR
    DMA1_Channel5->CMAR = (uint32_t) msg; // msg base address
    DMA1_Channel5->CNDTR = 8; // 8 objects
    DMA1_Channel5->CCR &= ~DMA_CCR_TCIE; // no done int
    DMA1_Channel5->CCR &= ~DMA_CCR_HTIE; // no half done int
    DMA1_Channel5->CCR &= ~DMA_CCR_TEIE; // no error int
    DMA1_Channel5->CCR |= DMA_CCR_DIR; // mem->per
    DMA1_Channel5->CCR |= DMA_CCR_CIRC; // circular
    DMA1_Channel5->CCR &= ~DMA_CCR_PINC; // no CPAR inc
    DMA1_Channel5->CCR |= DMA_CCR_MINC; // CMAR inc
    DMA1_Channel5->CCR |= DMA_CCR_PSIZE_0; // 16-bit
    DMA1_Channel5->CCR |= DMA_CCR_MSIZE_0; // 16-bit
    DMA1_Channel5->CCR &= ~DMA_CCR_PL; // low priority (00)
    DMA1_Channel5->CCR &= ~DMA_CCR_MEM2MEM; // not mem->mem
}

void enable_dma(void)
{
    DMA1_Channel5->CCR |= DMA_CCR_EN;
}

void print(const char str[])
{
    const char *p = str;
    for(int i=0; i<8; i++) {
        if (*p == '\0') {
            msg[i] = (i<<8);
        } else {
            msg[i] = (i<<8) | font[*p & 0x7f] | (*p & 0x80);
            p++;
        }
    }
}

int score = 0;
int main(void)
{
    print("Score  0");
    init_spi2();
    setup_dma();
    enable_dma();
    for (;;) {
        score += 1;
        char buf[9];
        snprintf(buf, 9, "Score% 3d", score);
        print(buf);
    }
}
