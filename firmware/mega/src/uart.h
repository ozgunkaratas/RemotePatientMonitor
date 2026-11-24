#ifndef RPM_UART_H
#define RPM_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* USART0 (the USB serial link to the Pi), 8N1, interrupt-driven transmit.
 * The main loop never waits for the line: bytes go into a ring buffer and the
 * data-register-empty interrupt drains it. */
void uart_init(uint32_t baud);

/* Queue bytes for sending. Returns false (and queues nothing) if the buffer
 * lacks room, so a slow or absent reader can never stall the firmware. */
bool uart_write(const uint8_t *data, size_t len);
bool uart_write_str(const char *s);

/* Bytes dropped because the transmit buffer was full. */
uint16_t uart_tx_dropped(void);

#endif
