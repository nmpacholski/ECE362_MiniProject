
#include "stm32f0xx.h"
#include <stdint.h>
#include "midi.h"
#include "midiplay.h"

// The number of simultaneous voices to support.
#define VOICES 15

// An array of "voices".  Each voice can be used to play a different note.
// Each voice can be associated with a channel (explained later).
// Each voice has a step size and an offset into the wave table.
struct {
    uint8_t in_use;
    uint8_t note;
    uint8_t chan;
    uint8_t volume;
    int     step;
    int     offset;
} voice[VOICES];

void init_EXTI(void){
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	GPIOA->MODER &= ~GPIO_MODER_MODER0;
	GPIOA->PUPDR |= GPIO_PUPDR_PUPDR0_1;
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR0_0;
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;
	SYSCFG->EXTICR[1] &= ~SYSCFG_EXTICR1_EXTI0;
	EXTI->RTSR |= EXTI_RTSR_TR0;
	EXTI->IMR |= EXTI_IMR_MR0;
	NVIC->ISER[0] |= 1<<EXTI0_1_IRQn;
}

void EXTI0_1_IRQHandler(void){
	EXTI->PR |= EXTI_PR_PR0;
	GPIOC->ODR |= GPIO_ODR_6;
	MIDI_Player *mp2 = midi_init(death);
	while(mp2->nexttick != MAXTICKS){};
}



// We'll use the Timer 6 IRQ to recompute samples and feed those
// samples into the DAC.
void TIM6_DAC_IRQHandler(void)
{
	//TODO: update interrupt flag
	TIM6->SR &= ~TIM_SR_UIF;

    int sample = 0;
    for(int x=0; x < sizeof voice / sizeof voice[0]; x++) {
        if (voice[x].in_use) {
            voice[x].offset += voice[x].step;
            if (voice[x].offset >= N<<16)
                voice[x].offset -= N<<16;
            sample += (wavetable[voice[x].offset>>16] * voice[x].volume) >> 4;
        }
    }
    sample = (sample >> 10) + 2048;
    if (sample > 4095)
        sample = 4095;
    else if (sample < 0)
        sample = 0;
    DAC->DHR12R1 = sample;
}

// Initialize the DAC so that it can output analog samples
// on PA4.  Configure it to be triggered by TIM6 TRGO.
void init_dac(void)
{
    // TODO: you fill this in.
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	GPIOA->MODER |= GPIO_MODER_MODER4;
	RCC->APB1ENR |= RCC_APB1ENR_DACEN;
	DAC->CR &= ~DAC_CR_EN1;
	DAC->CR &= ~DAC_CR_BOFF1;
	DAC->CR &= ~DAC_CR_TSEL1;
	DAC->CR |= DAC_CR_TEN1;
	DAC->CR |= DAC_CR_EN1;
}

// Initialize Timer 6 so that it calls TIM6_DAC_IRQHandler
// at exactly RATE times per second.  You'll need to select
// a PSC value and then do some math on the system clock rate
// to determine the value to set for ARR.  Set it to trigger
// the DAC by enabling the Update Trigger in the CR2 MMS field.
void init_tim6(void)
{
    // TODO: you fill this in.
	RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
	TIM6->PSC = 1-1;
	TIM6->ARR = 48000000 / ((TIM6->PSC+1) * RATE) - 1;
	TIM6->DIER |= TIM_DIER_UIE;
	TIM6->CR2 |= TIM_CR2_MMS_1;
	TIM6->CR1 |= TIM_CR1_CEN;
	NVIC->ISER[0] |= 1<<TIM6_DAC_IRQn;
}

// Find the voice current playing a note, and turn it off.
void note_off(int time, int chan, int key, int velo)
{
    int n;
    for(n=0; n<sizeof voice / sizeof voice[0]; n++) {
        if (voice[n].in_use && voice[n].note == key) {
            voice[n].in_use = 0; // disable it first...
            voice[n].chan = 0;   // ...then clear its values
            voice[n].note = key;
            voice[n].step = step[key];
            return;
        }
    }
}

// Find an unused voice, and use it to play a note.
void note_on(int time, int chan, int key, int velo)
{
    if (velo == 0) {
        note_off(time, chan, key, velo);
        return;
    }
    int n;
    for(n=0; n<sizeof voice / sizeof voice[0]; n++) {
        if (voice[n].in_use == 0) {
            voice[n].note = key;
            voice[n].step = step[key];
            voice[n].offset = 0;
            voice[n].volume = velo;
            voice[n].chan = chan;
            voice[n].in_use = 1;
            return;
        }
    }
}

void set_tempo(int time, int value, const MIDI_Header *hdr)
{
    // This assumes that the TIM2 prescaler divides by 48.
    // It sets the timer to produce an interrupt every N
    // microseconds, where N is the new tempo (value) divided by
    // the number of divisions per beat specified in the MIDI file header.
    TIM2->ARR = value/hdr->divisions - 1;
}

void TIM2_IRQHandler(void)
{
    // TODO: Remember to acknowledge the interrupt right here!
	TIM2->SR &= ~TIM_SR_UIF;
    midi_play();
}

// Configure timer 2 so that it invokes the Update interrupt
// every n microseconds.  To do so, set the prescaler to divide
// by 48.  Then the CNT will count microseconds up to the ARR value.
// Basically ARR = n-1
// Set the ARPE bit in the CR1 so that the timer waits until the next
// update before changing the effective ARR value.
// Call NVIC_SetPriority() to set a low priority for Timer 2 interrupt.
// See the lab 6 text to understand how to do so.
void init_tim2(int n) {
    // TODO: you fill this in.
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	TIM2->ARR = n-1;
	TIM2->PSC = 48-1;
	TIM2->CR1 |= TIM_CR1_ARPE;
	TIM2->DIER |= TIM_DIER_UIE;
	TIM2->CR1 |= TIM_CR1_CEN;
	NVIC->ISER[0] |= 1<<TIM2_IRQn;
}

int main(void)
{
    init_wavetable_hybrid2();
    init_dac();
    init_tim6();
    init_EXTI();
    NVIC_SetPriority(TIM6_DAC_IRQn, 0);
    NVIC_SetPriority(TIM2_IRQn, 1);
    NVIC_SetPriority(EXTI0_1_IRQn, 2);
    MIDI_Player *mp1 = midi_init(background);
    // The default rate for a MIDI file is 2 beats per second
    // with 48 ticks per beat.  That's 500000/48 microseconds.
    init_tim2(10417);
    for(;;) {
        asm("wfi");
        // If we hit the end of the MIDI file, start over.
        if (mp1->nexttick == MAXTICKS)
        	mp1 = midi_init(background);
    }
}
