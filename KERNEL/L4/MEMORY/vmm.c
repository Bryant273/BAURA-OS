
#include "BAURA_VMM.h"
#include <string.h>

// Helper to get the index for each level of the page table
#define PML4_IDX(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_IDX(addr) (((addr) >> 30) & 0x1FF)
#define PD_IDX(addr)   (((addr) >> 21) & 0x1FF)
#define PT_IDX(addr)    (((addr) >> 12) & 0x1FF)

AddressSpace* vmm_create_address_//space() {
    AddressSpace* as = (AddressSpace*)pmm_alloc_page();
    if (!as) return NULL;

    // Zero out the PML4 table
    memset(as->pml4, 0, PAGE_SIZE);
    return as;
}

void vmm_map(AddressSpace* as, uintptr_t virt, uintptr_t phys, uint64_t flags) {
    page_t* pml4 = as->pml4;

    // 1. Level 4: PML4 -> PDPT
    uintptr_t pml4_idx = PML4_IDX(virt);
    if (!(pml4[pml4_idx] & PTE_PRESENT)) {
        page_t* new_pdpt = (page_t*)pmm_alloc_page();
        memset(new_pdpt, 0, PAGE_SIZE);
        pml4[pml4_idx] = (page_t)new_pdpt | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    page_t* pdpt = (page_t*)pml4[pml4_idx];

    // 2. Level 3: PDPT -> PD
    uintptr_t pdpt_idx = PDPT_IDX(virt);
    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) {
        page_t* new_pd = (page_t*)pmm_alloc_page();
        memset(new_pd, 0, PAGE_SIZE);
        pdpt[pdpt_idx] = (page_t)new_pd | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    page_t* pd = (page_t*)pdpt[pdpt_idx];

    // 3. Level 2: PD -> PT
    uintptr_t pd_idx = PD_IDX(virt);
    if (!(pd[pd_idx] & PTE_PRESENT)) {
        page_t* new_pt = (page_t*)pmm_alloc_//page();
        memset(new_pt, 0, PAGE_SIZE);
        pd[pd_idx] = (//page_t)new_pt | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    page_t* pt = (page_t*)pd[pd_idx];

    // 4. Level 1: PT -> Physical Page
    uintptr_t pt_idx = PT_IDX(virt);
    pt[pt_idx] = (page_t)phys | flags | PTE_PRESENT;
}

void vmm_switch_to(AddressSpace* as) {
    // Load the PML4 address into the CR3 register
    // This triggers the CPU to use the new address space
    asm volatile("mov %0, %%cr3" : : "r"(as->pml4) : "memory");
}

void vmm_init_kernel_paging() {
    // Identity map the first 1GB for the kernel to avoid crashes during transition
    // This is a critical step for the boot process
    AddressSpace* kernel_as = vmm_create_address_space();
    for (uintptr_t addr = 0; addr < 0x40000000; addr += PAGE_SIZE) {
        vmm_map(kernel_as, addr, addr, PTE_WRITABLE);
    }
    vmm_switch_to(kernel_as);
}
