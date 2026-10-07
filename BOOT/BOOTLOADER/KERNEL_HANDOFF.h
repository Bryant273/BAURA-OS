#ifndef BAURA_KERNEL_HANDOFF_H
#define BAURA_KERNEL_HANDOFF_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BAURA_BOOT_HANDOFF_MAGIC UINT64_C(0x4241555241424F54)
#define BAURA_BOOT_HANDOFF_ABI_VERSION 1u

typedef struct {
    uint64_t magic;
    uint32_t abi_version;
    uint32_t struct_size;
    uintptr_t kernel_entry;
    const void *platform_data;
    size_t platform_data_size;
} BauraBootInfo;

/* validate returns exactly 1 only when the complete handoff is trusted. */
typedef int (*BauraBootValidate)(void *context, const BauraBootInfo *info);
typedef int (*BauraKernelTransfer)(void *context, uintptr_t entry,
                                   const BauraBootInfo *info);

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    BauraBootValidate validate;
    BauraKernelTransfer transfer;
    void *context;
} BauraBootHandoffBackend;

enum {
    BAURA_BOOT_OK = 0,
    BAURA_BOOT_ERR_ARGUMENT = -1,
    BAURA_BOOT_ERR_ABI = -2,
    BAURA_BOOT_ERR_VALIDATION = -3,
    BAURA_BOOT_ERR_TRANSFER = -4
};

int baura_boot_handoff(const BauraBootInfo *info,
                       const BauraBootHandoffBackend *backend);

#ifdef __cplusplus
}
#endif

#endif
