/*
 * Host tests for BAR sizing and untyped lookup, plus the port-backend write.
 * The fake device models BAR registers: bits below the size granularity read
 * as zero after an all-ones write, flag bits are read-only, and the status
 * half of the command dword is write-one-to-clear.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../SEL4/pci_bar.h"
#include "../SEL4/pci_backend.h"

typedef struct {
    uint32_t cfg[64];
    uint32_t bar_mask[6];   /* writable bits of each BAR dword */
    uint32_t bar_ro[6];     /* read-only flag bits */
    uint32_t status;        /* status register (bits 31:16 of dword 1) */
    unsigned writes;
    int fail_write_after;   /* fail the Nth write (1-based); 0 = never */
    int fail_all_reads;
    int decode_was_enabled_during_sizing;
} Dev;

static int dev_read(uint8_t b, uint8_t s, uint8_t f, uint8_t off, uint32_t *v, void *c) {
    Dev *d = c; (void)b; (void)s; (void)f;
    if (d->fail_all_reads) return 0;
    *v = d->cfg[off / 4u];
    if (off == 4u) *v = (d->cfg[1] & 0xFFFFu) | (d->status << 16);
    return 1;
}

static int dev_write(uint8_t b, uint8_t s, uint8_t f, uint8_t off, uint32_t v, void *c) {
    Dev *d = c; (void)b; (void)s; (void)f;
    ++d->writes;
    if (d->fail_write_after && (int)d->writes == d->fail_write_after) return 0;
    if (off == 4u) {
        d->status &= ~(v >> 16);               /* write-one-to-clear */
        d->cfg[1] = v & 0xFFFFu;
    } else if (off >= 0x10u && off < 0x28u) {
        unsigned i = (off - 0x10u) / 4u;
        if (v == 0xFFFFFFFFu && (d->cfg[1] & 3u) != 0)
            d->decode_was_enabled_during_sizing = 1;
        d->cfg[off / 4u] = (v & d->bar_mask[i]) | d->bar_ro[i];
    }
    return 1;
}

static void dev_init(Dev *d) {
    memset(d, 0, sizeof *d);
    d->cfg[0] = 0x100e8086u;
    d->cfg[1] = 0x0007u;       /* I/O, memory, bus master enabled */
    d->status = 0x0210u;       /* some write-one-to-clear bits set */
    d->cfg[3] = 0x00000000u;   /* header type 0 */
}

static BauraPciCfgOps ops_for(Dev *d) {
    BauraPciCfgOps o; o.read = dev_read; o.write = dev_write; o.context = d; return o;
}

static void test_mem32_io_and_restore(void) {
    Dev d; BauraPciCfgOps o; BauraPciBar bars[6];
    dev_init(&d); o = ops_for(&d);
    /* BAR0: 128 KiB prefetchable-less 32-bit memory at 0xFEBC0000 */
    d.bar_mask[0] = 0xFFFE0000u; d.bar_ro[0] = 0x0; d.cfg[4] = 0xFEBC0000u;
    /* BAR1: 64-byte I/O at 0xC040 */
    d.bar_mask[1] = 0xFFFFFFC0u; d.bar_ro[1] = 0x1; d.cfg[5] = 0xC041u;
    assert(baura_pci_bar_probe(&o, 0, 3, 0, bars) == BAURA_OK);
    assert(bars[0].kind == BAURA_BAR_MEM32 && bars[0].size == 0x20000u &&
           bars[0].base == 0xFEBC0000u && !bars[0].prefetchable);
    assert(bars[1].kind == BAURA_BAR_IO && bars[1].size == 0x40u && bars[1].base == 0xC040u);
    assert(bars[2].kind == BAURA_BAR_NONE);
    /* everything restored, including the write-one-to-clear status bits */
    assert(d.cfg[4] == 0xFEBC0000u && d.cfg[5] == 0xC041u);
    assert(d.cfg[1] == 0x0007u);
    assert(d.status == 0x0210u);          /* status was not cleared */
    assert(!d.decode_was_enabled_during_sizing);
}

static void test_mem64(void) {
    Dev d; BauraPciCfgOps o; BauraPciBar bars[6];
    dev_init(&d); o = ops_for(&d);
    /* BAR2/3: 64-bit prefetchable, 16 MiB at 0x1_E000_0000 */
    d.bar_mask[2] = 0xFF000000u; d.bar_ro[2] = 0xCu; d.cfg[6] = 0xE000000Cu;
    d.bar_mask[3] = 0xFFFFFFFFu; d.cfg[7] = 0x1u;
    assert(baura_pci_bar_probe(&o, 0, 4, 0, bars) == BAURA_OK);
    assert(bars[2].kind == BAURA_BAR_MEM64 && bars[2].prefetchable);
    assert(bars[2].size == 0x1000000u && bars[2].base == 0x1E0000000ull);
    assert(bars[3].kind == BAURA_BAR_NONE);
    assert(d.cfg[6] == 0xE000000Cu && d.cfg[7] == 1u);
    /* 64-bit BAR whose upper half is not implemented (reads back 0) */
    dev_init(&d); o = ops_for(&d);
    d.bar_mask[0] = 0xFFF00000u; d.bar_ro[0] = 0x4u; d.cfg[4] = 0xFE000004u;
    d.bar_mask[1] = 0u;
    assert(baura_pci_bar_probe(&o, 0, 4, 0, bars) == BAURA_OK);
    assert(bars[0].kind == BAURA_BAR_MEM64 && bars[0].size == 0x100000u);
}

static void test_errors_and_restore(void) {
    Dev d; BauraPciCfgOps o; BauraPciBar bars[6];
    int n;
    dev_init(&d); o = ops_for(&d);
    assert(baura_pci_bar_probe(0, 0, 0, 0, bars) == BAURA_ERR_INVALID_ARGUMENT);
    assert(baura_pci_bar_probe(&o, 0, 32, 0, bars) == BAURA_ERR_INVALID_ARGUMENT);
    assert(baura_pci_bar_probe(&o, 0, 0, 8, bars) == BAURA_ERR_INVALID_ARGUMENT);
    /* absent function */
    d.cfg[0] = 0xFFFFFFFFu;
    assert(baura_pci_bar_probe(&o, 0, 1, 0, bars) == BAURA_ERR_INVALID_STATE);
    /* bridge (header type 1) is refused without any write */
    dev_init(&d); o = ops_for(&d); d.cfg[3] = 0x00010000u;
    assert(baura_pci_bar_probe(&o, 0, 1, 0, bars) == BAURA_ERR_INVALID_STATE);
    assert(d.writes == 0);
    /* read failure before any write */
    dev_init(&d); o = ops_for(&d); d.fail_all_reads = 1;
    assert(baura_pci_bar_probe(&o, 0, 1, 0, bars) == BAURA_ERR_IO && d.writes == 0);
    /* 64-bit BAR in the last slot is invalid; state still restored */
    dev_init(&d); o = ops_for(&d);
    d.bar_mask[5] = 0xFFFFFFF0u; d.cfg[9] = 0x4u; d.bar_ro[5] = 0x4u;
    assert(baura_pci_bar_probe(&o, 0, 1, 0, bars) == BAURA_ERR_INVALID_STATE);
    assert(d.cfg[1] == 7u && d.cfg[9] == 4u);
    /* a failing write at every possible position never leaves decode off
     * silently: either the whole call succeeds or the original values are back
     * (command restore is the final write, so only a failure of that very
     * write can leave decoding disabled, and it is reported as an error) */
    for (n = 1; n <= 12; ++n) {
        BauraStatus st;
        dev_init(&d); o = ops_for(&d);
        d.bar_mask[0] = 0xFFFE0000u; d.cfg[4] = 0xFEBC0000u;
        d.fail_write_after = n;
        st = baura_pci_bar_probe(&o, 0, 3, 0, bars);
        if (st == BAURA_OK) continue;
        assert(st == BAURA_ERR_IO);
        assert(d.cfg[4] == 0xFEBC0000u);
        assert(bars[0].kind == BAURA_BAR_NONE);
    }
}

static void test_untyped(void) {
    BauraUntypedDesc u[3];
    u[0].paddr = 0x100000; u[0].size_bits = 20; u[0].is_device = 0;   /* RAM */
    u[1].paddr = 0xFEBC0000ull; u[1].size_bits = 17; u[1].is_device = 1;
    u[2].paddr = 0xFD000000ull; u[2].size_bits = 24; u[2].is_device = 1;
    assert(baura_untyped_find_device(u, 3, 0xFEBC0000ull, 0x20000u) == 1);
    assert(baura_untyped_find_device(u, 3, 0xFEBC0000ull, 0x20001u) == -1);
    assert(baura_untyped_find_device(u, 3, 0xFD100000ull, 0x1000u) == 2);
    assert(baura_untyped_find_device(u, 3, 0x100000ull, 0x1000u) == -1); /* RAM */
    assert(baura_untyped_find_device(u, 3, 0, 0) == -1);
    assert(baura_untyped_find_device(u, 3, UINT64_MAX - 1, 16) == -1);   /* wraps */
    assert(baura_untyped_find_device(0, 0, 0xFD000000ull, 16) == -1);
}

/* The port backend write path, against a trivial fake of ports 0xCF8/0xCFC. */
typedef struct { uint32_t address; uint32_t data_written; uint32_t address_at_write; int writes; } Ports;
static BauraStatus p_out(void *c, uint16_t port, uint32_t v) {
    Ports *p = c;
    if (port == BAURA_PCI_CONFIG_ADDRESS_PORT) p->address = v;
    else { p->data_written = v; p->address_at_write = p->address; ++p->writes; }
    return BAURA_OK;
}
static BauraStatus p_in(void *c, uint16_t port, uint32_t *v) { (void)c; (void)port; *v = 0; return BAURA_OK; }

static void test_backend_write(void) {
    Ports p; BauraPciPortIo io; BauraPciPortBackend b;
    memset(&p, 0, sizeof p);
    io.out32 = p_out; io.in32 = p_in; io.context = &p;
    assert(baura_pci_port_backend_init(&b, &io) == BAURA_OK);
    assert(baura_pci_port_config_write(0, 3, 1, 0x10, 0xDEADBEEFu, &b) == 1);
    assert(p.data_written == 0xDEADBEEFu);
    assert(p.address_at_write == baura_pci_config_address(0, 3, 1, 0x10));
    assert(baura_pci_port_config_write(0, 32, 0, 0x10, 0, &b) == 0);
    assert(baura_pci_port_config_write(0, 0, 8, 0x10, 0, &b) == 0);
    assert(baura_pci_port_config_write(0, 0, 0, 0x11, 0, &b) == 0);
    assert(baura_pci_port_config_write(0, 0, 0, 0x10, 0, 0) == 0);
    assert(p.writes == 1);
}

int main(void) {
    test_mem32_io_and_restore();
    test_mem64();
    test_errors_and_restore();
    test_untyped();
    test_backend_write();
    puts("pci_bar_test: all tests passed");
    return 0;
}
