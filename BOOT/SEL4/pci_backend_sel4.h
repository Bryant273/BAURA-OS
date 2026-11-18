#ifndef BAURA_PCI_BACKEND_SEL4_H
#define BAURA_PCI_BACKEND_SEL4_H

#include <sel4/sel4.h>
#include <sel4/bootinfo_types.h>

#include "../../KERNEL/BAURA_CORE/baura_core_types.h"

/*
 * Binds the PCI configuration backend (pci_backend.c) to seL4 on x86_64.
 *
 * Issues, from the initial task's IOPortControl capability, one I/O-port
 * capability for ports 0xCF8-0xCFF into the CNode slot *next_free_slot, then
 * installs the resulting reader with dev_set_pci_reader(). On success
 * *next_free_slot is advanced past the slot that was used, so the caller keeps
 * a single running free-slot cursor.
 *
 * Results:
 *   BAURA_OK                  backend installed
 *   BAURA_ERR_INVALID_ARGUMENT  null argument, or seL4 rejected the arguments
 *   BAURA_ERR_CAPACITY        *next_free_slot is outside boot_info->empty
 *   BAURA_ERR_PERMISSION      the kernel did not grant I/O-port authority
 *   BAURA_ERR_ALREADY_EXISTS  already installed, or the ports were already issued
 *   BAURA_ERR_IO              any other seL4 error
 */
BauraStatus baura_sel4_pci_install(const seL4_BootInfo *boot_info,
                                   seL4_SlotPos *next_free_slot);

#endif
