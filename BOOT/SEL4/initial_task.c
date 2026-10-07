#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <sel4/bootinfo_types.h>

#include "../../KERNEL/BAURA_CORE/SYSTEM_API/baura_core_entry.h"

static int valid_slot_region(seL4_SlotRegion region, seL4_Word cap_limit) {
    return region.start <= region.end && region.end <= cap_limit;
}

static BauraStatus validate_sel4_boot_info(void *context,
                                           const void *opaque_boot_info) {
    (void)context;
    if (opaque_boot_info == NULL ||
        ((uintptr_t)opaque_boot_info % seL4_BootInfoFrameSize) != 0) {
        return BAURA_ERR_INVALID_ARGUMENT;
    }

    const seL4_BootInfo *boot_info =
        (const seL4_BootInfo *)opaque_boot_info;
    const seL4_Word word_bits = (seL4_Word)(sizeof(seL4_Word) * CHAR_BIT);
    const seL4_Word cnode_bits = boot_info->initThreadCNodeSizeBits;
    if (boot_info->numNodes == 0 || boot_info->nodeID >= boot_info->numNodes ||
        boot_info->ipcBuffer == NULL || cnode_bits == 0 ||
        cnode_bits >= word_bits) {
        return BAURA_ERR_INVALID_ARGUMENT;
    }

    const seL4_Word cap_limit = (seL4_Word)1 << cnode_bits;
    if (cap_limit < seL4_NumInitialCaps ||
        !valid_slot_region(boot_info->empty, cap_limit) ||
        !valid_slot_region(boot_info->sharedFrames, cap_limit) ||
        !valid_slot_region(boot_info->userImageFrames, cap_limit) ||
        !valid_slot_region(boot_info->userImagePaging, cap_limit) ||
        !valid_slot_region(boot_info->ioSpaceCaps, cap_limit) ||
        !valid_slot_region(boot_info->extraBIPages, cap_limit) ||
#ifdef CONFIG_KERNEL_MCS
        !valid_slot_region(boot_info->schedcontrol, cap_limit) ||
#endif
        !valid_slot_region(boot_info->untyped, cap_limit)) {
        return BAURA_ERR_INVALID_ARGUMENT;
    }
    return BAURA_OK;
}

static BauraStatus sel4_console_write(void *context, const char *text,
                                     size_t length) {
    (void)context;
    if (text == NULL && length != 0) return BAURA_ERR_INVALID_ARGUMENT;
    for (size_t index = 0; index < length; ++index) {
        seL4_DebugPutChar(text[index]);
    }
    return BAURA_OK;
}

__attribute__((noreturn))
void baura_sel4_initial_task(seL4_BootInfo *boot_info) {
    BauraCorePlatform platform = {0};
    platform.abi_version = BAURA_CORE_PLATFORM_ABI_VERSION;
    platform.struct_size = sizeof(platform);
    platform.boot_info = boot_info;
    platform.validate_boot_info = validate_sel4_boot_info;
    platform.console_write = sel4_console_write;

    if (baura_core_initial_task_start(&platform) != BAURA_OK) {
        static const char failure[] = "BAURA_INIT: Core startup failed\r\n";
        (void)sel4_console_write(NULL, failure, sizeof(failure) - 1u);
    }

    for (;;) seL4_Yield();
}
