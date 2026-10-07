#include "../KERNEL_HANDOFF.h"

int baura_boot_handoff(const BauraBootInfo *info,
                       const BauraBootHandoffBackend *backend) {
    if (info == 0 || backend == 0) return BAURA_BOOT_ERR_ARGUMENT;
    if (info->magic != BAURA_BOOT_HANDOFF_MAGIC ||
        info->abi_version != BAURA_BOOT_HANDOFF_ABI_VERSION ||
        info->struct_size < sizeof(*info) ||
        info->kernel_entry == 0 ||
        (info->platform_data_size != 0 && info->platform_data == 0) ||
        backend->abi_version != BAURA_BOOT_HANDOFF_ABI_VERSION ||
        backend->struct_size < sizeof(*backend) ||
        backend->validate == 0 || backend->transfer == 0) {
        return BAURA_BOOT_ERR_ABI;
    }
    if (backend->validate(backend->context, info) != 1) {
        return BAURA_BOOT_ERR_VALIDATION;
    }
    return backend->transfer(backend->context, info->kernel_entry, info) == 0
               ? BAURA_BOOT_OK
               : BAURA_BOOT_ERR_TRANSFER;
}
