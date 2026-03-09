#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <openssl/sha.h>

/* ─── Hex lookup table ─────────────────────────────────────────────── */
static uint16_t HEX16[256];

static void init_hex_table(void) {
    const char *h = "0123456789abcdef";
    for (int i = 0; i < 256; i++)
        HEX16[i] = (uint8_t)h[i >> 4] | ((uint16_t)(uint8_t)h[i & 15] << 8);
}

/* ─── SHA1 state (h0-h4) → 40 hex chars ───────────────────────────── */
static inline void state_to_hex(const SHA_CTX *restrict ctx,
                                 char out[restrict 40])
{
    uint16_t *p = (uint16_t *)out;
    const uint32_t h[5] = { ctx->h0, ctx->h1, ctx->h2, ctx->h3, ctx->h4 };
    for (int i = 0; i < 5; i++) {
        uint32_t v = h[i];
        p[4*i+0] = HEX16[(v >> 24) & 0xFF];
        p[4*i+1] = HEX16[(v >> 16) & 0xFF];
        p[4*i+2] = HEX16[(v >>  8) & 0xFF];
        p[4*i+3] = HEX16[ v        & 0xFF];
    }
}

int main(int argc, char *argv[]) {
    init_hex_table();

    /* ── Precompute SHA1 padding สำหรับ 40-byte input ──────────────────
       Layout: [msg 40B][0x80][22 zeros][length 8B big-endian]
       Length = 40 × 8 = 320 bits = 0x0000000000000140               */
    static uint8_t block[64] __attribute__((aligned(64)));
    block[40] = 0x80;
    /* block[41..55] = 0x00 (จาก static) */
    /* block[56..61] = 0x00 */
    block[62] = 0x01;
    block[63] = 0x40;

    char msg[41];
    SHA_CTX ctx;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rounds>\n", argv[0]);
        return 1;
    }
    const uint64_t ROUNDS = strtoull(argv[1], NULL, 10);

    /* ── Round 0: "clubhouse" (9 bytes) ─── ใช้ API ปกติ ─────────── */
    {
        unsigned char digest[20];
        SHA1((const unsigned char *)"clubhouse", 9, digest);
        uint16_t *p = (uint16_t *)msg;
        for (int i = 0; i < 20; i++) p[i] = HEX16[digest[i]];
    }

    /* ── Rounds 1…N: 40 bytes เสมอ ─────────────────────────────────── */
    for (uint64_t i = 1; i < ROUNDS; i++) {

        /* [1] ใส่ message 40 bytes (padding ที่เหลือ precompute แล้ว) */
        memcpy(block, msg, 40);

        /* [2] Init state โดยตรง — ไม่เรียก SHA1_Init() */
        ctx.h0 = 0x67452301u;
        ctx.h1 = 0xEFCDAB89u;
        ctx.h2 = 0x98BADCFEu;
        ctx.h3 = 0x10325476u;
        ctx.h4 = 0xC3D2E1F0u;

        /* [3] Single-block compression โดยตรง — ไม่เรียก Update/Final */
        SHA1_Transform(&ctx, block);

        /* [4] State → hex (ไม่ผ่าน digest array) */
        state_to_hex(&ctx, msg);
    }

    msg[40] = '\0';
    printf("%s\n", msg);
    return 0;
}
