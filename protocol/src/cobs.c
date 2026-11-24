#include "rpm/protocol.h"

size_t rpm_cobs_encode(const uint8_t *in, size_t len, uint8_t *out, size_t out_cap)
{
    if (out_cap < RPM_COBS_MAX_ENCODED(len)) {
        return 0;
    }

    size_t code_pos = 0; /* where the current block's length code goes */
    size_t write = 1;
    uint8_t code = 1;    /* distance to the next zero (or block end) */

    for (size_t i = 0; i < len; ++i) {
        if (in[i] == 0U) {
            out[code_pos] = code;
            code_pos = write++;
            code = 1;
        } else {
            out[write++] = in[i];
            ++code;
            if (code == 0xFFU) { /* block full: 254 data bytes, no implied zero */
                out[code_pos] = code;
                if (i + 1U == len) {
                    return write; /* input ends exactly at a block boundary */
                }
                code_pos = write++;
                code = 1;
            }
        }
    }
    out[code_pos] = code;
    return write;
}

size_t rpm_cobs_decode(const uint8_t *in, size_t len, uint8_t *out, size_t out_cap)
{
    size_t read = 0;
    size_t write = 0;

    while (read < len) {
        const uint8_t code = in[read++];
        if (code == 0U) {
            return 0; /* a zero can never appear inside an encoded frame */
        }
        for (uint8_t i = 1; i < code; ++i) {
            if (read >= len || write >= out_cap || in[read] == 0U) {
                return 0;
            }
            out[write++] = in[read++];
        }
        /* Every block except a full (0xFF) one and the last implies a zero. */
        if (code != 0xFFU && read < len) {
            if (write >= out_cap) {
                return 0;
            }
            out[write++] = 0U;
        }
    }
    return write;
}
