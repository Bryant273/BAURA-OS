#include "pci_bar.h"

#define CFG_ID 0x00u
#define CFG_COMMAND 0x04u
#define CFG_HEADER 0x0Cu
#define CFG_BAR0 0x10u
#define CMD_DECODE_MASK 0x0003u /* I/O space (bit 0) and memory space (bit 1) */

typedef struct {
    const BauraPciCfgOps *ops;
    uint8_t bus, slot, function;
} Fn;

static int rd(const Fn *f, uint8_t off, uint32_t *v) {
    return f->ops->read(f->bus, f->slot, f->function, off, v, f->ops->context);
}
static int wr(const Fn *f, uint8_t off, uint32_t v) {
    return f->ops->write(f->bus, f->slot, f->function, off, v, f->ops->context);
}
/* Restoring original values matters more than anything else here, so a failed
 * restore is retried once before it is reported. */
static int wr_restore(const Fn *f, uint8_t off, uint32_t v) {
    return wr(f, off, v) || wr(f, off, v);
}

/* Writes all ones to one BAR dword, reads the result, then restores `orig`.
 * Returns 1 only if every access succeeded; the restore is attempted anyway. */
static int probe_dword(const Fn *f, uint8_t off, uint32_t orig, uint32_t *mask) {
    int ok = wr(f, off, 0xFFFFFFFFu);
    ok = ok && rd(f, off, mask);
    if (!wr_restore(f, off, orig)) ok = 0;
    return ok;
}

BauraStatus baura_pci_bar_probe(const BauraPciCfgOps *ops, uint8_t bus, uint8_t slot,
                                uint8_t function, BauraPciBar bars[6]) {
    Fn f;
    uint32_t id, header, command_dword, orig[6], i;
    BauraStatus status = BAURA_OK;
    int restored_command = 0;

    if (ops == 0 || ops->read == 0 || ops->write == 0 || bars == 0 ||
        slot >= 32u || function >= 8u)
        return BAURA_ERR_INVALID_ARGUMENT;
    f.ops = ops; f.bus = bus; f.slot = slot; f.function = function;
    for (i = 0; i < 6u; ++i) {
        bars[i].kind = BAURA_BAR_NONE; bars[i].index = (uint8_t)i;
        bars[i].prefetchable = 0; bars[i].base = 0; bars[i].size = 0;
    }

    if (!rd(&f, CFG_ID, &id) || !rd(&f, CFG_HEADER, &header) ||
        !rd(&f, CFG_COMMAND, &command_dword))
        return BAURA_ERR_IO;
    if ((id & 0xFFFFu) == 0xFFFFu) return BAURA_ERR_INVALID_STATE; /* absent */
    if (((header >> 16) & 0x7Fu) != 0) return BAURA_ERR_INVALID_STATE; /* not type 0 */
    for (i = 0; i < 6u; ++i)
        if (!rd(&f, (uint8_t)(CFG_BAR0 + 4u * i), &orig[i])) return BAURA_ERR_IO;

    /* Bits 31:16 of this dword are the status register, whose bits are
     * write-one-to-clear: always write the status half as zero. */
    command_dword &= 0xFFFFu;
    if (!wr(&f, CFG_COMMAND, command_dword & ~CMD_DECODE_MASK)) return BAURA_ERR_IO;

    for (i = 0; i < 6u && status == BAURA_OK; ++i) {
        uint8_t off = (uint8_t)(CFG_BAR0 + 4u * i);
        uint32_t mask;
        if (orig[i] & 1u) { /* I/O space */
            if (!probe_dword(&f, off, orig[i], &mask)) { status = BAURA_ERR_IO; break; }
            mask &= ~3u;
            if (mask == 0) continue;
            {
                uint32_t size = (~mask + 1u);
                if ((mask >> 16) == 0) size &= 0xFFFFu; /* 16-bit I/O BAR */
                bars[i].kind = BAURA_BAR_IO;
                bars[i].base = orig[i] & ~3u;
                bars[i].size = size;
            }
        } else {
            uint32_t type = (orig[i] >> 1) & 3u;
            uint8_t prefetch = (uint8_t)((orig[i] >> 3) & 1u);
            if (type == 3u) continue; /* reserved encoding: ignore */
            if (type == 2u) {         /* 64-bit memory BAR */
                uint32_t hi_orig, lo_mask, hi_mask;
                if (i == 5u) { status = BAURA_ERR_INVALID_STATE; break; }
                hi_orig = orig[i + 1u];
                if (!probe_dword(&f, off, orig[i], &lo_mask) ||
                    !probe_dword(&f, (uint8_t)(off + 4u), hi_orig, &hi_mask)) {
                    status = BAURA_ERR_IO; break;
                }
                lo_mask &= ~0xFu;
                if (lo_mask == 0 && hi_mask == 0) { ++i; continue; }
                bars[i].kind = BAURA_BAR_MEM64;
                bars[i].prefetchable = prefetch;
                bars[i].base = ((uint64_t)hi_orig << 32) | (orig[i] & ~0xFu);
                if (hi_mask == 0) {
                    bars[i].size = (uint64_t)(uint32_t)(~lo_mask + 1u);
                    if (bars[i].size == 0) bars[i].size = UINT64_C(1) << 32;
                } else {
                    uint64_t m = ((uint64_t)hi_mask << 32) | lo_mask;
                    bars[i].size = ~m + 1u;
                }
                ++i; /* the upper dword belongs to this BAR; stays NONE */
            } else {                  /* 32-bit memory BAR (type 1 = below 1 MiB) */
                if (!probe_dword(&f, off, orig[i], &mask)) { status = BAURA_ERR_IO; break; }
                mask &= ~0xFu;
                if (mask == 0) continue;
                bars[i].kind = BAURA_BAR_MEM32;
                bars[i].prefetchable = prefetch;
                bars[i].base = orig[i] & ~0xFu;
                bars[i].size = (uint32_t)(~mask + 1u);
            }
        }
    }

    restored_command = wr_restore(&f, CFG_COMMAND, command_dword);
    if (!restored_command && status == BAURA_OK) status = BAURA_ERR_IO;
    if (status != BAURA_OK) {
        for (i = 0; i < 6u; ++i) {
            bars[i].kind = BAURA_BAR_NONE; bars[i].base = 0; bars[i].size = 0;
            bars[i].prefetchable = 0;
        }
    }
    return status;
}

int baura_untyped_find_device(const BauraUntypedDesc *list, uint32_t count,
                              uint64_t base, uint64_t size) {
    uint32_t i;
    uint64_t end;
    if (list == 0 || size == 0) return -1;
    end = base + size;
    if (end < base) return -1; /* wraps around */
    for (i = 0; i < count; ++i) {
        uint64_t ut_size, ut_end;
        if (!list[i].is_device || list[i].size_bits >= 63u) continue;
        ut_size = UINT64_C(1) << list[i].size_bits;
        ut_end = list[i].paddr + ut_size;
        if (ut_end < list[i].paddr) continue;
        if (base >= list[i].paddr && end <= ut_end) return (int)i;
    }
    return -1;
}
