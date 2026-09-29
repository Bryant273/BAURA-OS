
#ifndef BAURA_VMM_H
#define BAURA_VMM_H

#include <stdint.h>
#include <stddef.h>
#include "pmm_full.h"

// Page Table Entry Flags
#define PTE_PRESENT  (1 << 0)
#define PTE_WRITABLE (1 << 1)
#define PTE_USER     (1 << 2)
#define PTE_PWT      (1 << 3)
#define PTE_PCD      (1 << 4)
#define PTE_ACCESSED (1 << 5)
#define PTE_DIRTY    (1 << 6)
#define PTE_HUGE     (1 << 7)

typedef uint64_t page_t;

typedef struct {
    page_t* pml4;
} AddressSpace;

// Core functions
AddressSpace* vmm_create_address_space();
void vmm_map(AddressSpace* as, uintptr_t virt, uintptr_t phys, uint64_t flags);
void vmm_unmap(AddressSpace* as, uintptr_t virt);
void vmm_switch_to(AddressSpace* as);
void vmm_init_kernel_paging();

#endif
