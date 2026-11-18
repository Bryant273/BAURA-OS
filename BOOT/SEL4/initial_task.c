#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <sel4/bootinfo_types.h>

#include "../../KERNEL/BAURA_CORE/DEVICE_MANAGER/baura_device_report.h"
#include "../../KERNEL/BAURA_CORE/HARDWARE_MANAGER/baura_hardware_manager.h"
#include "../../KERNEL/BAURA_CORE/SYSTEM_API/baura_core_entry.h"
#include "../FIRMWARE/FRAMEBUFFER/baura_boot_splash.h"
#include "../FIRMWARE/FRAMEBUFFER/baura_desktop_manager.h"
#include "baura_ps2_sel4.h"
#include "pci_backend_sel4.h"

/* -----------------------------------------------------------------------
 * seL4 TLS buffer (LibSel4UseThreadLocals=OFF → LIBSEL4_THREAD_LOCAL is void)
 * ---------------------------------------------------------------------- */
LIBSEL4_THREAD_LOCAL seL4_IPCBuffer *__sel4_ipc_buffer = 0;
#ifdef CONFIG_KERNEL_INVOCATION_REPORT_ERROR_IPC
LIBSEL4_THREAD_LOCAL char __sel4_print_error = 0;
#endif

/* -----------------------------------------------------------------------
 * Multiboot2 framebuffer chunk (mirrors seL4 x86 bootinfo extension)
 * ---------------------------------------------------------------------- */
typedef struct __attribute__((packed)) {
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t  bpp;
    uint8_t  type;
} BauraSel4Multiboot2Fb;

static BauraBootFramebuffer g_boot_fb = {0};
static int g_fb_detected = 0;

/* -----------------------------------------------------------------------
 * Minimal console helpers (no libc)
 * ---------------------------------------------------------------------- */
static void sel4_print_char(char c) {
    seL4_DebugPutChar(c);
}

static void sel4_print(const char *text) {
    if (text == 0) return;
    for (size_t i = 0; text[i] != '\0'; ++i) {
        sel4_print_char(text[i]);
    }
}

static void sel4_print_dec(uint32_t value) {
    char buf[12];
    int pos = 0;
    if (value == 0) {
        sel4_print_char('0');
        return;
    }
    while (value > 0) {
        buf[pos++] = (char)('0' + (value % 10));
        value /= 10;
    }
    while (pos > 0) {
        sel4_print_char(buf[--pos]);
    }
}

static void sel4_print_hex64(uint64_t value) {
    sel4_print("0x");
    for (int shift = 60; shift >= 0; shift -= 4) {
        const uint8_t nibble = (uint8_t)((value >> shift) & 0x0Fu);
        sel4_print_char(nibble < 10 ? (char)('0' + nibble) : (char)('A' + nibble - 10));
    }
}

/* Shared console_write implementation used by the BauraCorePlatform callback
 * and the PCI reporting helper below. */
static BauraStatus sel4_console_write(void *context, const char *text,
                                     size_t length) {
    (void)context;
    if (text == NULL && length != 0) return BAURA_ERR_INVALID_ARGUMENT;
    for (size_t index = 0; index < length; ++index) {
        seL4_DebugPutChar(text[index]);
    }
    return BAURA_OK;
}

/* console_puts used by report_hardware() — keeps a single free-function style */
static void console_puts(const char *text) {
    size_t length = 0;
    while (text[length] != '\0') ++length;
    (void)sel4_console_write(NULL, text, length);
}

static void console_put_int(int value) {
    char digits[12];
    size_t count = 0;
    unsigned int magnitude = value < 0 ? 0u - (unsigned int)value : (unsigned int)value;
    char text[14];
    size_t out = 0;
    do {
        digits[count++] = (char)('0' + (magnitude % 10u));
        magnitude /= 10u;
    } while (magnitude != 0u);
    if (value < 0) text[out++] = '-';
    while (count > 0) text[out++] = digits[--count];
    text[out] = '\0';
    console_puts(text);
}

static void console_put_failure(const char *what, BauraStatus status) {
    console_puts("BAURA_INIT: ");
    console_puts(what);
    console_puts(" (status ");
    console_put_int((int)status);
    console_puts(")\r\n");
}

/* -----------------------------------------------------------------------
 * Boot-info helper
 * ---------------------------------------------------------------------- */
static int valid_slot_region(seL4_SlotRegion region, seL4_Word cap_limit) {
    return region.start <= region.end && region.end <= cap_limit;
}

static int parse_sel4_bootinfo_framebuffer(const seL4_BootInfo *boot_info,
                                           BauraSel4Multiboot2Fb *out_fb) {
    if (boot_info == 0 || boot_info->extraLen == 0 || out_fb == 0) {
        return 0;
    }
    const uintptr_t extra_base = (uintptr_t)boot_info + seL4_BootInfoFrameSize;
    uintptr_t offset = 0;

    while (offset + sizeof(seL4_BootInfoHeader) <= boot_info->extraLen) {
        const seL4_BootInfoHeader *header =
            (const seL4_BootInfoHeader *)(extra_base + offset);
        if (header->len < sizeof(seL4_BootInfoHeader) ||
            offset + header->len > boot_info->extraLen) {
            break;
        }
        if (header->id == SEL4_BOOTINFO_HEADER_X86_FRAMEBUFFER) {
            if (header->len >= sizeof(seL4_BootInfoHeader) + sizeof(BauraSel4Multiboot2Fb)) {
                const BauraSel4Multiboot2Fb *fb =
                    (const BauraSel4Multiboot2Fb *)(extra_base + offset + sizeof(seL4_BootInfoHeader));
                *out_fb = *fb;
                return 1;
            }
        }
        offset += header->len;
    }
    return 0;
}

/* -----------------------------------------------------------------------
 * Framebuffer mapping — 4K pages, allocates intermediate paging structures
 *
 * Strategy:
 *   1. Locate the device untyped covering fb_paddr.
 *   2. From RAM (non-device) untypeds, allocate one PDPT, one PD, one PT
 *      (only those missing for the target range at 0x20000000).
 *   3. Install PDPT → PD → PT into the initial VSpace.
 *   4. Retype 4K frames from the device untyped.
 *   5. Map each frame.
 *
 * All seL4_CPtr allocations advance a single free_slot cursor kept in
 * boot_info->empty.start order (we do not modify the BootInfo; we just
 * track the next free slot ourselves).
 * ---------------------------------------------------------------------- */

/* Find the first RAM (non-device) untyped >= min_size_bits.
 * Returns its cap, or 0 on failure. Advances *free_slot by 1 (retype creates
 * 1 object of the requested type). */
static seL4_CPtr alloc_paging_object(const seL4_BootInfo *boot_info,
                                     seL4_Word obj_type,
                                     seL4_Word *free_slot) {
    const seL4_Word num_untypeds =
        boot_info->untyped.end - boot_info->untyped.start;

    for (seL4_Word i = 0; i < num_untypeds; ++i) {
        const seL4_UntypedDesc *desc = &boot_info->untypedList[i];
        /* We need a RAM untyped large enough for the paging structure
         * (at least 12 bits = 4 KiB). */
        if (!desc->isDevice && desc->sizeBits >= 12) {
            seL4_CPtr ut_cap = boot_info->untyped.start + i;
            seL4_CPtr dest = *free_slot;
            seL4_Error err = seL4_Untyped_Retype(
                ut_cap,
                obj_type,
                0,
                seL4_CapInitThreadCNode,
                0, 0,
                dest,
                1
            );
            if (err == seL4_NoError) {
                *free_slot = dest + 1;
                return dest;
            }
            /* If this untyped is exhausted, try the next one. */
        }
    }
    return 0;
}

static void *map_framebuffer(const seL4_BootInfo *boot_info,
                             uint64_t fb_paddr,
                             uint32_t width,
                             uint32_t height,
                             uint32_t pitch,
                             uint8_t bpp,
                             seL4_SlotPos *free_slot_cursor) {
    (void)bpp;
    if (boot_info == 0 || fb_paddr == 0 || width == 0 || height == 0 || free_slot_cursor == 0) {
        return NULL;
    }

    const uint64_t page_size    = 4096ULL;
    const uint64_t total_size   = (uint64_t)pitch * height;
    const uint64_t num_pages    = (total_size + page_size - 1) / page_size;
    const uintptr_t target_vaddr = 0x20000000UL; /* 512 MiB mark */

    const seL4_Word num_untypeds =
        boot_info->untyped.end - boot_info->untyped.start;

    sel4_print("BAURA_INIT: Searching device untyped covering fb at ");
    sel4_print_hex64(fb_paddr);
    sel4_print(" (total=");
    sel4_print_dec((uint32_t)total_size);
    sel4_print(" bytes, pages=");
    sel4_print_dec((uint32_t)num_pages);
    sel4_print(")\r\n");

    /* 1. Find device untyped covering fb_paddr */
    seL4_CPtr device_ut_cap = 0;
    uint64_t  device_ut_paddr = 0;
    seL4_Word device_ut_size_bits = 0;

    for (seL4_Word i = 0; i < num_untypeds; ++i) {
        const seL4_UntypedDesc *desc = &boot_info->untypedList[i];
        if (desc->isDevice) {
            const uint64_t start = desc->paddr;
            const uint64_t size  = (uint64_t)1 << desc->sizeBits;
            if (fb_paddr >= start && fb_paddr < (start + size)) {
                device_ut_cap        = boot_info->untyped.start + i;
                device_ut_paddr      = start;
                device_ut_size_bits  = desc->sizeBits;
                break;
            }
        }
    }

    if (device_ut_cap == 0) {
        sel4_print("BAURA_INIT: No device untyped covers framebuffer.\r\n");
        return NULL;
    }

    sel4_print("BAURA_INIT: Device untyped at ");
    sel4_print_hex64(device_ut_paddr);
    sel4_print(" sizeBits=");
    sel4_print_dec((uint32_t)device_ut_size_bits);
    sel4_print("\r\n");

    /* Free-slot cursor — we advance it as we allocate caps. */
    seL4_Word free_slot = *free_slot_cursor;

    /* 2. Halve cur_ut until it starts exactly at fb_paddr */
    seL4_CPtr cur_ut_cap = device_ut_cap;
    uint64_t cur_ut_paddr = device_ut_paddr;
    seL4_Word cur_size_bits = device_ut_size_bits;

    while (cur_ut_paddr != fb_paddr && cur_size_bits > 21) {
        seL4_Word next_size_bits = cur_size_bits - 1;
        uint64_t half_size = (uint64_t)1 << next_size_bits;
        seL4_CPtr child_slot = free_slot;

        sel4_print("BAURA_INIT: Splitting untyped at ");
        sel4_print_hex64(cur_ut_paddr);
        sel4_print(" (sizeBits=");
        sel4_print_dec((uint32_t)cur_size_bits);
        sel4_print(") into 2 x ");
        sel4_print_dec((uint32_t)next_size_bits);
        sel4_print(" bits...\r\n");

        seL4_Error split_err = seL4_Untyped_Retype(
            cur_ut_cap,
            seL4_UntypedObject,
            next_size_bits,
            seL4_CapInitThreadCNode,
            0, 0,
            child_slot,
            2
        );
        if (split_err != seL4_NoError) {
            sel4_print("BAURA_INIT: Untyped split failed err=");
            sel4_print_dec((uint32_t)split_err);
            sel4_print("\r\n");
            return NULL;
        }
        free_slot += 2;

        if (fb_paddr >= cur_ut_paddr + half_size) {
            cur_ut_paddr += half_size;
            cur_ut_cap = child_slot + 1;
        } else {
            cur_ut_cap = child_slot;
        }
        cur_size_bits = next_size_bits;
    }

    if (cur_ut_paddr != fb_paddr) {
        sel4_print("BAURA_INIT: Could not align device untyped to fb_paddr.\r\n");
        return NULL;
    }

    sel4_print("BAURA_INIT: Aligned device untyped at ");
    sel4_print_hex64(cur_ut_paddr);
    sel4_print(" (sizeBits=");
    sel4_print_dec((uint32_t)cur_size_bits);
    sel4_print(")\r\n");

    /* 3. Allocate intermediate paging structures from RAM untypeds.
     *    For x86_64 with KernelHugePage=OFF and 2MB LargePage mapping at 0x20000000:
     *      PML4[0] already covers 0–512 GiB  (exists)
     *      PDPT[0] covers 0–1 GiB            (may not exist)
     *      PD[1]   covers 0x20000000–0x3FFFFFFF (entry 256 in PD)
     *    No PageTable is required for 2MB pages (they map directly in the PD). */

    /* Allocate PDPT */
    seL4_CPtr pdpt_cap = alloc_paging_object(boot_info, seL4_X86_PDPTObject, &free_slot);
    if (pdpt_cap == 0) {
        sel4_print("BAURA_INIT: Failed to allocate PDPT object.\r\n");
        return NULL;
    }

    /* Map PDPT at vaddr 0x20000000 — seL4 derives PML4 index from the vaddr */
    seL4_Error err = seL4_X86_PDPT_Map(
        pdpt_cap,
        seL4_CapInitThreadVSpace,
        target_vaddr,
        seL4_X86_Default_VMAttributes
    );
    if (err != seL4_NoError && err != seL4_DeleteFirst) {
        sel4_print("BAURA_INIT: PDPT_Map err=");
        sel4_print_dec((uint32_t)err);
        sel4_print("\r\n");
        return NULL;
    }

    /* Allocate PageDirectory */
    seL4_CPtr pd_cap = alloc_paging_object(boot_info, seL4_X86_PageDirectoryObject, &free_slot);
    if (pd_cap == 0) {
        sel4_print("BAURA_INIT: Failed to allocate PD object.\r\n");
        return NULL;
    }

    err = seL4_X86_PageDirectory_Map(
        pd_cap,
        seL4_CapInitThreadVSpace,
        target_vaddr,
        seL4_X86_Default_VMAttributes
    );
    if (err != seL4_NoError && err != seL4_DeleteFirst) {
        sel4_print("BAURA_INIT: PD_Map err=");
        sel4_print_dec((uint32_t)err);
        sel4_print("\r\n");
        return NULL;
    }

    sel4_print("BAURA_INIT: Paging structures installed.\r\n");

    /* 4. Retype cur_ut_cap into 2MB LargePage frame caps.
     *    1024x768x32 framebuffer = 3,145,728 bytes = 2 x 2MB pages (4MB). */
    const uint64_t large_page_size = 2ULL * 1024ULL * 1024ULL;
    const uint32_t num_large_pages = (uint32_t)((total_size + large_page_size - 1) / large_page_size);

    seL4_CPtr frame_base_cap = free_slot;
    seL4_Error retype_err = seL4_Untyped_Retype(
        cur_ut_cap,
        seL4_X86_LargePageObject,
        0,
        seL4_CapInitThreadCNode,
        0, 0,
        frame_base_cap,
        num_large_pages
    );
    if (retype_err != seL4_NoError) {
        sel4_print("BAURA_INIT: LargePage retype failed err=");
        sel4_print_dec((uint32_t)retype_err);
        sel4_print("\r\n");
        return NULL;
    }
    free_slot += num_large_pages;

    /* 5. Map each 2MB large page directly into the PageDirectory. */
    for (uint32_t p = 0; p < num_large_pages; ++p) {
        seL4_CPtr page_cap = frame_base_cap + p;
        seL4_Word  vaddr   = (seL4_Word)(target_vaddr + (uint64_t)p * large_page_size);
        seL4_Error map_err = seL4_X86_Page_Map(
            page_cap,
            seL4_CapInitThreadVSpace,
            vaddr,
            seL4_ReadWrite,
            seL4_X86_Default_VMAttributes
        );
        if (map_err != seL4_NoError) {
            sel4_print("BAURA_INIT: LargePage Map[");
            sel4_print_dec(p);
            sel4_print("] err=");
            sel4_print_dec((uint32_t)map_err);
            sel4_print("\r\n");
            return NULL;
        }
    }

    sel4_print("BAURA_INIT: Framebuffer mapped to virtual address ");
    sel4_print_hex64(target_vaddr);
    sel4_print("\r\n");
    *free_slot_cursor = free_slot;
    return (void *)target_vaddr;
}

/* -----------------------------------------------------------------------
 * BauraCorePlatform callbacks
 * ---------------------------------------------------------------------- */
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

static BauraStatus sel4_start_graphical_service(void *context) {
    BauraBootFramebuffer *fb = (BauraBootFramebuffer *)context;
    if (fb != NULL && g_fb_detected) {
        sel4_print("BAURA_INIT: Starting graphical boot splash...\r\n");
        if (fb->pixels != NULL) {
            baura_boot_draw_splash(fb, "BAURA OS x86_64 INITIALIZING", 45);
            sel4_print("BAURA_INIT: BAURA Boot image displayed on screen.\r\n");
        } else {
            sel4_print("BAURA_INIT: Framebuffer physical address recorded; awaiting user-space page map.\r\n");
        }
    }
    return BAURA_OK;
}

static BauraStatus sel4_authority_check(void *context, uint64_t subject_id,
                                       const char *capability_name) {
    (void)context;
    if (capability_name == 0) return BAURA_ERR_INVALID_ARGUMENT;
    /* Subject 0 (initial boot task) and subject 1 (root service manager) hold core authority */
    if (subject_id <= 1u) {
        return BAURA_OK;
    }
    return BAURA_ERR_PERMISSION;
}

/* -----------------------------------------------------------------------
 * PCI hardware report (integrated from BAURA_drivers_complet_v2)
 * ---------------------------------------------------------------------- */
static void report_hardware(const seL4_BootInfo *boot_info,
                            seL4_SlotPos next_free_slot) {
    BauraStatus status = baura_sel4_pci_install(boot_info, &next_free_slot);
    if (status != BAURA_OK) {
        console_put_failure("PCI backend unavailable", status);
        return;
    }
    hardware_manager_init();
    status = baura_hardware_refresh();
    if (status != BAURA_OK) {
        console_put_failure("PCI enumeration failed", status);
        return;
    }
    console_puts("BAURA_INIT: PCI devices found: ");
    console_put_int((int)dev_device_count());
    console_puts("\r\n");
    for (uint32_t index = 0; index < dev_device_count(); ++index) {
        BauraDevice device;
        char line[BAURA_DEVICE_REPORT_LINE_MAX];
        if (dev_get_device(index, &device) != BAURA_OK) break;
        if (baura_device_format_line(&device, line, sizeof(line)) != 0) {
            console_puts(line);
        }
    }
}

/* -----------------------------------------------------------------------
 * Initial task entry point
 * ---------------------------------------------------------------------- */
__attribute__((noreturn))
void baura_sel4_initial_task(seL4_BootInfo *boot_info) {
    /* Install IPC buffer immediately — required before any seL4 syscall. */
    if (boot_info != NULL && boot_info->ipcBuffer != NULL) {
        seL4_SetIPCBuffer(boot_info->ipcBuffer);
    }

    sel4_print("BAURA_INIT: Starting x86_64 initial task...\r\n");

    seL4_SlotPos free_slot_cursor = boot_info->empty.start;

    /* Parse Multiboot2 framebuffer info from seL4 BootInfo extras. */
    BauraSel4Multiboot2Fb mb_fb = {0};
    if (parse_sel4_bootinfo_framebuffer(boot_info, &mb_fb)) {
        g_fb_detected = 1;
        g_boot_fb.physical_address = mb_fb.addr;
        g_boot_fb.width  = mb_fb.width;
        g_boot_fb.height = mb_fb.height;
        g_boot_fb.pitch  = mb_fb.pitch;
        g_boot_fb.bpp    = mb_fb.bpp;
        g_boot_fb.format = BAURA_BOOT_FB_FORMAT_RGBX;

        sel4_print("BAURA_INIT: Framebuffer detected: ");
        sel4_print_dec(mb_fb.width);
        sel4_print("x");
        sel4_print_dec(mb_fb.height);
        sel4_print("@");
        sel4_print_dec((uint32_t)mb_fb.bpp);
        sel4_print(" at ");
        sel4_print_hex64(mb_fb.addr);
        sel4_print(" (pitch=");
        sel4_print_dec(mb_fb.pitch);
        sel4_print(" type=");
        sel4_print_dec((uint32_t)mb_fb.type);
        sel4_print(")\r\n");

        if (mb_fb.type == 1 || mb_fb.bpp == 32) {
            g_boot_fb.pixels = map_framebuffer(boot_info, mb_fb.addr,
                                               mb_fb.width, mb_fb.height,
                                               mb_fb.pitch, mb_fb.bpp,
                                               &free_slot_cursor);
            if (g_boot_fb.pixels != NULL) {
                sel4_print("BAURA_INIT: Starting animated graphical boot splash...\r\n");
                (void)baura_boot_draw_splash(&g_boot_fb, "INITIALISATION DU MICRO-NOYAU seL4 x86_64", 15);
                for (volatile int d = 0; d < 4000000; ++d);
                baura_boot_update_progress(&g_boot_fb, "PAGINATION ET MAPPING DE LA VRAM (0x20000000)", 35);
                for (volatile int d = 0; d < 4000000; ++d);
                sel4_print("BAURA_INIT: BAURA Boot image displayed on screen.\r\n");
            }
        } else {
            g_boot_fb.pixels = NULL;
            sel4_print("BAURA_INIT: Non-linear or text framebuffer mode, skipping direct pixel mapping.\r\n");
        }
    } else {
        sel4_print("BAURA_INIT: No Multiboot2 framebuffer chunk found in BootInfo.\r\n");
    }

    if (g_boot_fb.pixels != NULL) {
        baura_boot_update_progress(&g_boot_fb, "DEMARRAGE DES SERVICES BAURA CORE", 55);
    }

    /* Build the platform descriptor and hand off to BAURA Core. */
    BauraCorePlatform platform = {0};
    platform.abi_version              = BAURA_CORE_PLATFORM_ABI_VERSION;
    platform.struct_size              = sizeof(platform);
    platform.boot_info                = boot_info;
    platform.validate_boot_info       = validate_sel4_boot_info;
    platform.console_write            = sel4_console_write;
    platform.authority_check          = sel4_authority_check;
    platform.start_graphical_service  = sel4_start_graphical_service;
    platform.graphical_service_context = &g_boot_fb;

    if (baura_core_initial_task_start(&platform) != BAURA_OK) {
        static const char failure[] = "BAURA_INIT: Core startup failed\r\n";
        (void)sel4_console_write(NULL, failure, sizeof(failure) - 1u);
    }

    if (g_boot_fb.pixels != NULL) {
        baura_boot_update_progress(&g_boot_fb, "EXPLORATION MATERIELLE BUS PCI", 75);
    }

    /* PCI hardware discovery — runs after core startup so that dev_register_*
     * callbacks installed by the core are in place.  Pass free_slot_cursor
     * which was advanced past any slots consumed by the framebuffer mapping. */
    report_hardware(boot_info, free_slot_cursor);

    if (g_boot_fb.pixels != NULL) {
        baura_boot_update_progress(&g_boot_fb, "CHARGEMENT DU RUNTIME GRAPHIQUE BGE", 90);
        for (volatile int d = 0; d < 6000000; ++d);
        baura_boot_update_progress(&g_boot_fb, "SYSTEME OPERATIONNEL - BIENVENUE", 100);
        for (volatile int d = 0; d < 10000000; ++d);
    }

    /* Initialisation du pilote de souris & clavier PS/2 matériel */
    BauraPs2Sel4 ps2_driver;
    int ps2_ok = (baura_ps2_sel4_install(&ps2_driver, boot_info, &free_slot_cursor) == 0);
    if (ps2_ok) {
        sel4_print("BAURA_INIT: PS/2 Mouse & Keyboard initialized successfully via seL4 IOPort.\r\n");
    } else {
        sel4_print("BAURA_INIT: PS/2 direct port issue returned non-zero.\r\n");
    }

    /* Initialisation du BDS / Gestionnaire de fenêtres interactif */
    BauraDesktopManager desktop_mgr;
    if (g_boot_fb.pixels != NULL) {
        sel4_print("BAURA_INIT: Boot sequence complete. Launching interactive BDS Desktop...\r\n");
        baura_desktop_init(&desktop_mgr, (int)g_boot_fb.width, (int)g_boot_fb.height);
        baura_desktop_render_full(&g_boot_fb, &desktop_mgr);
        sel4_print("BAURA_INIT: BAURA OS interactive desktop live on screen with mouse cursor.\r\n");
    }

    /* Boucle interactive temps réel : gestionnaire d'événements souris et fenêtres */
    int tick = 0;
    for (;;) {
        seL4_Yield();
        tick++;

        if (g_boot_fb.pixels != NULL) {
            int dx = 0, dy = 0;
            uint8_t btns = 0;
            int packets = 0;
            if (ps2_ok) {
                packets = baura_ps2_sel4_poll(&ps2_driver, &dx, &dy, &btns);
            }
            if (packets > 0) {
                baura_desktop_handle_mouse(&g_boot_fb, &desktop_mgr, dx, dy, btns);
            }
        }
    }
}
