#include "tick.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

static volatile uint32_t s_ms;

void tick_init(void)
{
    /* CTC mode (WGM12), prescaler 64: 16 MHz / 64 = 250 kHz -> 250 counts per ms. */
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);
    OCR1A = (F_CPU / 64UL / 1000UL) - 1U;
    TCNT1 = 0;
    TIMSK1 = (1 << OCIE1A);
}

ISR(TIMER1_COMPA_vect)
{
    s_ms++;
}

uint32_t tick_ms(void)
{
    uint32_t ms;
    /* A 32-bit read is four byte loads on AVR: block the ISR so we never see a torn value. */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        ms = s_ms;
    }
    return ms;
}
