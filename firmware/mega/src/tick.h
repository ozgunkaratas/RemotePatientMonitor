#ifndef RPM_TICK_H
#define RPM_TICK_H

#include <stdint.h>

/* 1 kHz system tick from Timer1 in CTC mode.
 * Timer1 is reserved for timing: later phases sample the waveforms from this ISR. */
void tick_init(void);

/* Milliseconds since tick_init(). Safe to call from the main loop. */
uint32_t tick_ms(void);

#endif
