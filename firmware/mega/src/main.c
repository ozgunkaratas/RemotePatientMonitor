/* rpm Mega firmware — phase 0
 *
 * Proves the toolchain and the USB serial link:
 *   - Timer1 1 kHz tick (the time base later phases sample from)
 *   - on-board LED (D13) blinks at 1 Hz
 *   - a text heartbeat every 100 ms:  "HB seq=<n> uptime_ms=<ms> drop=<bytes>\n"
 *
 * Phase 1 replaces the text heartbeat with COBS/CRC frames from protocol/.
 */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>

#include "tick.h"
#include "uart.h"

#define BAUD 115200UL
#define HEARTBEAT_PERIOD_MS 100U
#define BLINK_HALF_PERIOD_MS 500U

/* D13 = PB7 on the ATmega2560 */
#define LED_DDR DDRB
#define LED_PORT PORTB
#define LED_BIT PB7

static void send_heartbeat(uint32_t seq, uint32_t uptime_ms)
{
    char line[64];
    char num[11];
    uint8_t n = 0;

    /* Build the line by hand: printf would pull in several kB of code. */
    const char *parts[] = {"HB seq=", NULL, " uptime_ms=", NULL, " drop=", NULL, "\n"};
    uint32_t values[] = {seq, uptime_ms, uart_tx_dropped()};
    uint8_t v = 0;

    for (uint8_t p = 0; p < sizeof parts / sizeof parts[0]; ++p) {
        const char *s = parts[p];
        if (s == NULL) {
            ultoa(values[v++], num, 10);
            s = num;
        }
        while (*s != '\0' && n < sizeof line - 1U) {
            line[n++] = *s++;
        }
    }
    uart_write((const uint8_t *)line, n);
}

int main(void)
{
    LED_DDR |= (1 << LED_BIT);

    tick_init();
    uart_init(BAUD);
    sei();

    uint32_t seq = 0;
    uint32_t next_hb = 0;
    uint32_t next_blink = 0;

    for (;;) {
        uint32_t now = tick_ms();

        /* Signed difference keeps the comparisons correct across the 49-day wrap. */
        if ((int32_t)(now - next_hb) >= 0) {
            send_heartbeat(seq++, now);
            next_hb += HEARTBEAT_PERIOD_MS;
        }
        if ((int32_t)(now - next_blink) >= 0) {
            LED_PORT ^= (1 << LED_BIT);
            next_blink += BLINK_HALF_PERIOD_MS;
        }
    }
}
