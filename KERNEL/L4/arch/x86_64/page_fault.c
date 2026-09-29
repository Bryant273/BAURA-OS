
#include "BAURA_VMM.h"
#include "BAURA_SECURITY_RUST.h"
#include <stdint.h>

void handle_page_fault(uintptr_t faulting_addr, uint64_t error_code) {
    // 1. Check if it's a protection violation or a missing page
    bool is_present = error_code & 0x1;
    bool is_write = error_code & 0x2;

    if (!is_present) {
        // Demand Paging: If the address is valid in the app's manifest, allocate it now
        if (sec_validate_capability(current_task->id, 0, CAP_MEM_ACCESS)) {
            vmm_map(current_task->as, faulting_addr & ~0xFFF, pmm_alloc_page(), PTE_WRITABLE | PTE_USER);
            return; // Retry the instruction
        }
    }

    // If we reach here, it's a real segmentation fault
    kernel_panic("Segmentation Fault: Invalid access to address");
}
