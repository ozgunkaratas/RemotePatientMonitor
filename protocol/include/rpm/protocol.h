#ifndef RPM_PROTOCOL_H
#define RPM_PROTOCOL_H

/* Shared Mega <-> Pi packet library. Plain C11, no allocation, builds with
 * avr-gcc (firmware) and gcc/g++ (gateway, host tests).
 *
 * Frame on the wire (see the "Communication protocols" section of the doc):
 *   COBS( ver | type | seq(2) | timestamp_us(4) | len | payload(len) | crc16(2) ) 0x00
 * Multi-byte fields are little-endian. The CRC covers everything before it and
 * is computed before COBS encoding.
 *
 * Phase 0 provides the two building blocks; framing follows in phase 1. */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RPM_PROTOCOL_VERSION 1U
#define RPM_MAX_PAYLOAD 64U

/* Worst-case COBS output size for n input bytes (excluding the 0x00 delimiter). */
#define RPM_COBS_MAX_ENCODED(n) ((n) + ((n) / 254U) + 1U)

/* CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflection, no final XOR.
 * Check value: crc16("123456789") == 0x29B1. */
uint16_t rpm_crc16(const uint8_t *data, size_t len);

/* Continue a CRC over more data (start with crc = 0xFFFF). */
uint16_t rpm_crc16_update(uint16_t crc, const uint8_t *data, size_t len);

/* COBS-encodes `len` bytes into `out` (capacity `out_cap`). The output holds
 * no 0x00 bytes; the caller appends the 0x00 frame delimiter.
 * Returns the encoded length, or 0 if `out_cap` is too small. */
size_t rpm_cobs_encode(const uint8_t *in, size_t len, uint8_t *out, size_t out_cap);

/* Decodes one COBS frame (without its 0x00 delimiter).
 * Returns the decoded length, or 0 on malformed input or if `out_cap` is too small. */
size_t rpm_cobs_decode(const uint8_t *in, size_t len, uint8_t *out, size_t out_cap);

#ifdef __cplusplus
}
#endif

#endif
