#include <stdint.h>
#include "SYSTEM_MANAGER/baura_system_manager.h"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002u
#define SERIAL_PORT 0x3F8u

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void serial_init(void) {
    outb(SERIAL_PORT + 1u, 0x00u);
    outb(SERIAL_PORT + 3u, 0x80u);
    outb(SERIAL_PORT, 0x03u);
    outb(SERIAL_PORT + 1u, 0x00u);
    outb(SERIAL_PORT + 3u, 0x03u);
    outb(SERIAL_PORT + 2u, 0xC7u);
    outb(SERIAL_PORT + 4u, 0x0Bu);
}

static void serial_write(const char *text) {
    while (*text != 0) {
        while ((inb(SERIAL_PORT + 5u) & 0x20u) == 0u) {
        }
        outb(SERIAL_PORT, (uint8_t)*text++);
    }
}

static __attribute__((noreturn)) void halt(void) {
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

void kernel_main(uint32_t magic, uint32_t multiboot_info) {
    (void)multiboot_info;
    serial_init();
    serial_write("BAURA-OS booting\r\n");

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        serial_write("ERROR: invalid Multiboot magic\r\n");
        halt();
    }

    if (baura_system_start() != BAURA_OK) {
        serial_write("ERROR: BAURA Core initialization failed\r\n");
        halt();
    }
    serial_write("CORE_READY: BAURA Core services initialized\r\n");
    halt();
}
