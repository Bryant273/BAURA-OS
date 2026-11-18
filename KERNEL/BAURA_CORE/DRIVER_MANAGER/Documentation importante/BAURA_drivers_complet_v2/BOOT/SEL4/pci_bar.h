#ifndef BAURA_PCI_BAR_H
#define BAURA_PCI_BAR_H

/*
 * PCI BAR sizing and untyped lookup. Portable: configuration access is
 * injected, there is no seL4 dependency. Mapping the resulting range into an
 * address space is NOT done here (see DRIVER_DOCS/09_BAR_ET_MAPPING.md).
 *
 * Sizing follows the usual PCI procedure: disable memory and I/O decoding in
 * the command register, write all ones to the BAR, read it back, restore the
 * BAR, restore the command register. Only header type 0 functions are sized.
 * Sizing briefly disables decoding of the device: do not size a device the
 * firmware or another agent is actively using (see the document above).
 */

#include <stdint.h>

#include "../../KERNEL/BAURA_CORE/baura_core_types.h"

typedef struct {
    /* Return 1 on success, 0 on failure; offset is dword-aligned. */
    int (*read)(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset,
                uint32_t *value, void *context);
    int (*write)(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset,
                 uint32_t value, void *context);
    void *context;
} BauraPciCfgOps;

typedef enum {
    BAURA_BAR_NONE = 0,   /* not implemented, or the upper half of a 64-bit BAR */
    BAURA_BAR_MEM32 = 1,
    BAURA_BAR_MEM64 = 2,
    BAURA_BAR_IO = 3
} BauraBarKind;

typedef struct {
    BauraBarKind kind;
    uint8_t index;          /* index of the (first) BAR dword, 0-5 */
    uint8_t prefetchable;
    uint64_t base;          /* as programmed by the firmware; 0 = unassigned */
    uint64_t size;          /* bytes; 0 when kind is NONE */
} BauraPciBar;

/*
 * Sizes the six BARs of a header-type-0 function and fills `bars` (6 entries,
 * indexed by BAR dword; the upper dword of a 64-bit BAR is reported as NONE).
 * On any configuration access failure it makes a best effort to restore the
 * original BAR and command values and returns BAURA_ERR_IO. Returns
 * BAURA_ERR_INVALID_ARGUMENT for null arguments, BAURA_ERR_INVALID_STATE for a
 * header type other than 0 or an invalid 64-bit BAR in the last slot.
 */
BauraStatus baura_pci_bar_probe(const BauraPciCfgOps *ops, uint8_t bus, uint8_t slot,
                                uint8_t function, BauraPciBar bars[6]);

/* One entry of seL4_BootInfo.untypedList, flattened. */
typedef struct {
    uint64_t paddr;
    uint8_t size_bits;
    uint8_t is_device;
} BauraUntypedDesc;

/*
 * Finds the device untyped that fully contains [base, base+size). Returns its
 * index in `list` or -1. The range must be non-empty, not wrap around, and
 * the untyped must be a device untyped. This only answers "which untyped",
 * not how to carve it (objects inside an untyped are allocated in order; see
 * the document).
 */
int baura_untyped_find_device(const BauraUntypedDesc *list, uint32_t count,
                              uint64_t base, uint64_t size);

#endif
