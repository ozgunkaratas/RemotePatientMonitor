#include "rpm/protocol.h"

uint16_t rpm_crc16_update(uint16_t crc, const uint8_t *data, size_t len)
{
    /* Bitwise version: 2 bytes of RAM, no table. Fast enough for ~2 kB/s on
     * the Mega; switch to a 512-byte table in flash if profiling says so. */
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (uint8_t bit = 0; bit < 8U; ++bit) {
            if ((crc & 0x8000U) != 0U) {
                crc = (uint16_t)((crc << 1) ^ 0x1021U);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}

uint16_t rpm_crc16(const uint8_t *data, size_t len)
{
    return rpm_crc16_update(0xFFFFU, data, len);
}
