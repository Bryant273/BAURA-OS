#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bootinfo_chunks.h"
#include "spsc_ring.h"
#include "driver_contract.h"
#include "virtio_pci.h"

static size_t put(uint8_t *b, size_t o, uint64_t id, const void *p, size_t n)
{
    uint64_t len = 16 + n;
    memcpy(b + o, &id, 8); memcpy(b + o + 8, &len, 8);
    if (n) memcpy(b + o + 16, p, n);
    return o + (size_t)len;
}

int main(void)
{
    uint8_t buf[256]; size_t o = 0; BauraBootFb fb, got; BauraBootChunk c;
    uint8_t odd[5] = {1,2,3,4,5}; /* rend le chunk suivant non aligné */
    memset(&fb, 0, sizeof fb);
    fb.addr = 0xFD000000ull; fb.pitch = 4096; fb.width = 1024;
    fb.height = 768; fb.bpp = 32; fb.type = 1;
    assert(sizeof fb == 22);
    o = put(buf, o, BAURA_BI_ID_X86_ACPI_RSDP, odd, sizeof odd);
    o = put(buf, o, BAURA_BI_ID_X86_FRAMEBUFFER, &fb, sizeof fb);
    assert(baura_bootinfo_get_fb(buf, o, &got) && got.addr == fb.addr
           && got.width == 1024 && got.bpp == 32);
    assert(!baura_bootinfo_find_chunk(buf, o, BAURA_BI_ID_FDT, &c));
    assert(!baura_bootinfo_get_fb(buf, o - 1, &got));     /* tronqué */
    { uint64_t bad = 8; memcpy(buf + 8, &bad, 8); }        /* len < 16 */
    assert(!baura_bootinfo_get_fb(buf, o, &got));

    { BauraSpscRing r; uint8_t in[64], out[64]; unsigned i;
      memset(&r, 0, sizeof r);
      for (i = 0; i < BAURA_RING_SLOTS; i++) { memset(in, (int)i, 64);
          assert(baura_ring_push(&r, in)); }
      assert(!baura_ring_push(&r, in));                    /* plein */
      for (i = 0; i < BAURA_RING_SLOTS; i++) { assert(baura_ring_pop(&r, out));
          assert(out[0] == (uint8_t)i); }
      assert(!baura_ring_pop(&r, out)); }

    { BauraMatchEntry exact = {0x8086, 0x100e, BAURA_ANY16, BAURA_ANY16,
                               BAURA_ANY8, BAURA_ANY8, BAURA_ANY8, 0, 255};
      BauraMatchEntry cls = {BAURA_ANY16, BAURA_ANY16, BAURA_ANY16, BAURA_ANY16,
                             0x02, BAURA_ANY8, BAURA_ANY8, 0, 255};
      int a = baura_match_score(&exact, 0x8086, 0x100e, 0, 0, 2, 0, 0, 3);
      int b = baura_match_score(&cls, 0x8086, 0x100e, 0, 0, 2, 0, 0, 3);
      assert(a > b && b >= 0);
      assert(baura_match_score(&exact, 0x10ec, 0x8168, 0, 0, 2, 0, 0, 3) < 0); }
    puts("driver docs examples: OK");
    return 0;
}
