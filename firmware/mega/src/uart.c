#include "uart.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <string.h>
#include <util/atomic.h>

#define TX_BUF_SIZE 256U /* power of two */
#define TX_MASK (TX_BUF_SIZE - 1U)

static uint8_t s_tx_buf[TX_BUF_SIZE];
static volatile uint8_t s_head; /* written by main loop */
static volatile uint8_t s_tail; /* written by ISR */
static volatile uint16_t s_dropped;

void uart_init(uint32_t baud)
{
    /* Double-speed mode gives the smallest error at 115200 on a 16 MHz clock
     * (UBRR = 16 -> 117647 baud, +2.1 %), the same setting the Arduino core uses. */
    UCSR0A = (1 << U2X0);
    uint16_t ubrr = (uint16_t)((F_CPU / (8UL * baud)) - 1UL);
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); /* 8 data bits, no parity, 1 stop */
}

static uint8_t free_space(void)
{
    /* One slot stays empty so head == tail always means "empty". */
    return (uint8_t)((s_tail - s_head - 1U) & TX_MASK);
}

bool uart_write(const uint8_t *data, size_t len)
{
    if (len == 0U) {
        return true;
    }
    if (len > free_space()) {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { s_dropped += (uint16_t)len; }
        return false;
    }
    uint8_t head = s_head;
    for (size_t i = 0; i < len; ++i) {
        s_tx_buf[head] = data[i];
        head = (uint8_t)((head + 1U) & TX_MASK);
    }
    s_head = head;               /* publish the bytes ... */
    UCSR0B |= (1 << UDRIE0);     /* ... then let the ISR start draining */
    return true;
}

bool uart_write_str(const char *s)
{
    return uart_write((const uint8_t *)s, strlen(s));
}

uint16_t uart_tx_dropped(void)
{
    uint16_t d;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { d = s_dropped; }
    return d;
}

ISR(USART0_UDRE_vect)
{
    uint8_t tail = s_tail;
    if (tail == s_head) {
        UCSR0B &= (uint8_t)~(1 << UDRIE0); /* nothing left: stop the interrupt */
        return;
    }
    UDR0 = s_tx_buf[tail];
    s_tail = (uint8_t)((tail + 1U) & TX_MASK);
}
