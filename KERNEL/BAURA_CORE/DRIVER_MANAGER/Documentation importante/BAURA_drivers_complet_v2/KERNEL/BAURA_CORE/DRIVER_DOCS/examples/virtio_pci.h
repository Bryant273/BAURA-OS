/* Exemple (proposition) : structures virtio 1.x sur PCI, d'après les en-têtes
 * Linux/U-Boot (voir 06_PLAN_VIRTIO_BLK.md pour l'état de vérification). */
#ifndef BAURA_VIRTIO_PCI_H
#define BAURA_VIRTIO_PCI_H
#include <stdint.h>

#define VIRTIO_PCI_VENDOR 0x1AF4u
#define VIRTIO_PCI_MODERN_ID_BASE 0x1040u
#define VIRTIO_PCI_CAP_VNDR 0x09u

#define VIRTIO_PCI_CAP_COMMON_CFG 1u
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2u
#define VIRTIO_PCI_CAP_ISR_CFG 3u
#define VIRTIO_PCI_CAP_DEVICE_CFG 4u
#define VIRTIO_PCI_CAP_PCI_CFG 5u

#define VIRTIO_STATUS_ACKNOWLEDGE 1u
#define VIRTIO_STATUS_DRIVER 2u
#define VIRTIO_STATUS_DRIVER_OK 4u
#define VIRTIO_STATUS_FEATURES_OK 8u
#define VIRTIO_STATUS_NEEDS_RESET 64u
#define VIRTIO_STATUS_FAILED 128u

#define VIRTIO_F_VERSION_1_BIT 32u

#define VIRTQ_DESC_F_NEXT 1u
#define VIRTQ_DESC_F_WRITE 2u
#define VIRTQ_DESC_F_INDIRECT 4u

#define VIRTIO_BLK_T_IN 0u
#define VIRTIO_BLK_T_OUT 1u
#define VIRTIO_BLK_T_FLUSH 4u
#define VIRTIO_BLK_S_OK 0u
#define VIRTIO_BLK_S_IOERR 1u
#define VIRTIO_BLK_S_UNSUPP 2u

typedef struct __attribute__((packed)) {
    uint8_t cap_vndr, cap_next, cap_len, cfg_type, bar, id, padding[2];
    uint32_t offset, length;
} VirtioPciCap;

typedef struct __attribute__((packed)) {
    VirtioPciCap cap;
    uint32_t notify_off_multiplier;
} VirtioPciNotifyCap;

typedef struct __attribute__((packed)) {
    uint32_t device_feature_select, device_feature;
    uint32_t guest_feature_select, guest_feature;
    uint16_t msix_config, num_queues;
    uint8_t device_status, config_generation;
    uint16_t queue_select, queue_size, queue_msix_vector, queue_enable,
             queue_notify_off;
    uint32_t queue_desc_lo, queue_desc_hi, queue_avail_lo, queue_avail_hi,
             queue_used_lo, queue_used_hi;
} VirtioPciCommonCfg;

typedef struct __attribute__((packed)) { uint64_t addr; uint32_t len;
    uint16_t flags, next; } VirtqDesc;
typedef struct __attribute__((packed)) { uint32_t type, ioprio;
    uint64_t sector; } VirtioBlkOutHdr;

_Static_assert(sizeof(VirtqDesc) == 16, "desc 16 octets");
_Static_assert(sizeof(VirtioBlkOutHdr) == 16, "outhdr 16 octets");
_Static_assert(sizeof(VirtioPciCap) == 16, "cap 16 octets");
_Static_assert(sizeof(VirtioPciCommonCfg) == 56, "common cfg 56 octets");
#endif
