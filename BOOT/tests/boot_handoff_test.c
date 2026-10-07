#include <assert.h>
#include <stdint.h>
#include "../BOOTLOADER/KERNEL_HANDOFF.h"

static uint32_t validations;
static uint32_t transfers;
static int validation_result = 1;
static int transfer_result;

static int validate(void *context, const BauraBootInfo *info) {
    assert(context == &validations);
    assert(info->kernel_entry == (uintptr_t)0x1000);
    ++validations;
    return validation_result;
}

static int transfer(void *context, uintptr_t entry, const BauraBootInfo *info) {
    (void)context;
    assert(entry == (uintptr_t)0x1000);
    assert(info->magic == BAURA_BOOT_HANDOFF_MAGIC);
    ++transfers;
    return transfer_result;
}

int main(void) {
    BauraBootInfo info = {
        BAURA_BOOT_HANDOFF_MAGIC,
        BAURA_BOOT_HANDOFF_ABI_VERSION,
        sizeof(BauraBootInfo),
        (uintptr_t)0x1000,
        0,
        0
    };
    BauraBootHandoffBackend backend = {
        BAURA_BOOT_HANDOFF_ABI_VERSION,
        sizeof(BauraBootHandoffBackend),
        validate,
        transfer,
        &validations
    };

    assert(baura_boot_handoff(0, &backend) == BAURA_BOOT_ERR_ARGUMENT);
    assert(baura_boot_handoff(&info, &backend) == BAURA_BOOT_OK);
    assert(validations == 1 && transfers == 1);
    validation_result = 0;
    assert(baura_boot_handoff(&info, &backend) == BAURA_BOOT_ERR_VALIDATION);
    assert(validations == 2 && transfers == 1);
    validation_result = -1;
    assert(baura_boot_handoff(&info, &backend) == BAURA_BOOT_ERR_VALIDATION);
    assert(validations == 3 && transfers == 1);
    validation_result = 1;
    transfer_result = -1;
    assert(baura_boot_handoff(&info, &backend) == BAURA_BOOT_ERR_TRANSFER);
    assert(validations == 4 && transfers == 2);
    info.magic = 0;
    assert(baura_boot_handoff(&info, &backend) == BAURA_BOOT_ERR_ABI);
    assert(validations == 4 && transfers == 2);
    return 0;
}
