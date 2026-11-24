/* Host unit tests for the protocol building blocks. Verifies: SYS-020. */
#include <stdio.h>
#include <string.h>

#include "rpm/protocol.h"

static int g_failures = 0;

#define CHECK(expr)                                                                   \
    do {                                                                              \
        if (!(expr)) {                                                                \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #expr);  \
            ++g_failures;                                                             \
        }                                                                             \
    } while (0)

static void crc_check_value(void)
{
    const uint8_t msg[] = "123456789";
    CHECK(rpm_crc16(msg, 9) == 0x29B1U);
    CHECK(rpm_crc16(msg, 0) == 0xFFFFU);
    /* Incremental CRC equals one-shot CRC. */
    uint16_t crc = rpm_crc16_update(0xFFFFU, msg, 4);
    crc = rpm_crc16_update(crc, msg + 4, 5);
    CHECK(crc == 0x29B1U);
}

static void crc_detects_single_bit_flip(void)
{
    uint8_t frame[16];
    for (unsigned i = 0; i < sizeof frame; ++i) {
        frame[i] = (uint8_t)(i * 37U);
    }
    const uint16_t good = rpm_crc16(frame, sizeof frame);
    for (unsigned byte = 0; byte < sizeof frame; ++byte) {
        for (unsigned bit = 0; bit < 8; ++bit) {
            frame[byte] ^= (uint8_t)(1U << bit);
            CHECK(rpm_crc16(frame, sizeof frame) != good);
            frame[byte] ^= (uint8_t)(1U << bit);
        }
    }
}

/* Known vectors from the COBS paper / Wikipedia. */
static void cobs_known_vectors(void)
{
    struct { uint8_t in[8]; size_t in_len; uint8_t out[10]; size_t out_len; } v[] = {
        {{0x00}, 1, {0x01, 0x01}, 2},
        {{0x00, 0x00}, 2, {0x01, 0x01, 0x01}, 3},
        {{0x11, 0x22, 0x00, 0x33}, 4, {0x03, 0x11, 0x22, 0x02, 0x33}, 5},
        {{0x11, 0x22, 0x33, 0x44}, 4, {0x05, 0x11, 0x22, 0x33, 0x44}, 5},
        {{0x11, 0x00, 0x00, 0x00}, 4, {0x02, 0x11, 0x01, 0x01, 0x01}, 5},
    };
    for (unsigned i = 0; i < sizeof v / sizeof v[0]; ++i) {
        uint8_t enc[16];
        const size_t n = rpm_cobs_encode(v[i].in, v[i].in_len, enc, sizeof enc);
        CHECK(n == v[i].out_len);
        CHECK(memcmp(enc, v[i].out, n) == 0);

        uint8_t dec[16];
        const size_t m = rpm_cobs_decode(enc, n, dec, sizeof dec);
        CHECK(m == v[i].in_len);
        CHECK(memcmp(dec, v[i].in, m) == 0);
    }
}

static void cobs_roundtrip_long_blocks(void)
{
    /* 600 bytes with no zeros crosses two 254-byte block boundaries. */
    static uint8_t in[600], enc[RPM_COBS_MAX_ENCODED(600)], dec[600];
    for (unsigned i = 0; i < sizeof in; ++i) {
        in[i] = (uint8_t)((i % 255U) + 1U);
    }
    const size_t n = rpm_cobs_encode(in, sizeof in, enc, sizeof enc);
    CHECK(n > 0U && n <= sizeof enc);
    CHECK(memchr(enc, 0, n) == NULL);
    CHECK(rpm_cobs_decode(enc, n, dec, sizeof dec) == sizeof in);
    CHECK(memcmp(in, dec, sizeof in) == 0);

    /* Same with zeros sprinkled in. */
    for (unsigned i = 0; i < sizeof in; i += 97U) {
        in[i] = 0;
    }
    const size_t n2 = rpm_cobs_encode(in, sizeof in, enc, sizeof enc);
    CHECK(memchr(enc, 0, n2) == NULL);
    CHECK(rpm_cobs_decode(enc, n2, dec, sizeof dec) == sizeof in);
    CHECK(memcmp(in, dec, sizeof in) == 0);
}

static void cobs_exact_block_boundary(void)
{
    /* 254 non-zero bytes encode as 0xFF + the bytes, with no trailing block. */
    uint8_t in[254], enc[RPM_COBS_MAX_ENCODED(254)], dec[254];
    for (unsigned i = 0; i < sizeof in; ++i) {
        in[i] = (uint8_t)(i + 1U);
    }
    const size_t n = rpm_cobs_encode(in, sizeof in, enc, sizeof enc);
    CHECK(n == 255U);
    CHECK(enc[0] == 0xFFU);
    CHECK(memcmp(enc + 1, in, sizeof in) == 0);
    CHECK(rpm_cobs_decode(enc, n, dec, sizeof dec) == sizeof in);
    CHECK(memcmp(in, dec, sizeof in) == 0);
}

static void cobs_rejects_bad_input(void)
{
    uint8_t out[16];
    const uint8_t has_zero[] = {0x03, 0x11, 0x00};
    const uint8_t truncated[] = {0x05, 0x11, 0x22};
    CHECK(rpm_cobs_decode(has_zero, sizeof has_zero, out, sizeof out) == 0U);
    CHECK(rpm_cobs_decode(truncated, sizeof truncated, out, sizeof out) == 0U);

    const uint8_t in[] = {1, 2, 3, 4};
    uint8_t tiny[3];
    CHECK(rpm_cobs_encode(in, sizeof in, tiny, sizeof tiny) == 0U);
}

int main(void)
{
    crc_check_value();
    crc_detects_single_bit_flip();
    cobs_known_vectors();
    cobs_roundtrip_long_blocks();
    cobs_exact_block_boundary();
    cobs_rejects_bad_input();

    if (g_failures != 0) {
        fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("all protocol tests passed\n");
    return 0;
}
