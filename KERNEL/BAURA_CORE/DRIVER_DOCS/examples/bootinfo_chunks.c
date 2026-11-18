#include "bootinfo_chunks.h"
#include <string.h>

#define HDR 16u /* seL4_BootInfoHeader = deux mots de 64 bits */

int baura_bootinfo_next_chunk(const uint8_t *extra, size_t extra_len,
                              size_t *offset, BauraBootChunk *out)
{
    uint64_t id, len;
    if (!extra || !offset || !out) return 0;
    if (*offset > extra_len || extra_len - *offset < HDR) return 0;
    memcpy(&id, extra + *offset, 8);
    memcpy(&len, extra + *offset + 8, 8);
    if (len < HDR || len > extra_len - *offset) return 0;
    out->id = id;
    out->len = len;
    out->payload = extra + *offset + HDR;
    out->payload_len = (size_t)(len - HDR);
    *offset += (size_t)len;
    return 1;
}

int baura_bootinfo_find_chunk(const uint8_t *extra, size_t extra_len,
                              uint64_t id, BauraBootChunk *out)
{
    size_t off = 0;
    BauraBootChunk c;
    while (baura_bootinfo_next_chunk(extra, extra_len, &off, &c)) {
        if (c.id == id) { *out = c; return 1; }
    }
    return 0;
}

int baura_bootinfo_get_fb(const uint8_t *extra, size_t extra_len,
                          BauraBootFb *fb)
{
    BauraBootChunk c;
    if (!fb) return 0;
    if (!baura_bootinfo_find_chunk(extra, extra_len,
                                   BAURA_BI_ID_X86_FRAMEBUFFER, &c)) return 0;
    if (c.payload_len < sizeof *fb) return 0;
    memcpy(fb, c.payload, sizeof *fb);
    return 1;
}
