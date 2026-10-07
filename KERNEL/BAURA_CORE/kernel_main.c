#include <stdint.h>
#include "SYSTEM_MANAGER/baura_system_manager.h"
#include "../../BOOT/FIRMWARE/UART/uart_driver.h"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002u

static int serial_port_write8(void *context, uint16_t port, uint8_t value) {
    (void)context;
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
    return 0;
}

static int serial_port_read8(void *context, uint16_t port, uint8_t *value) {
    (void)context;
    if (value == 0) return -1;
    __asm__ volatile("inb %1, %0" : "=a"(*value) : "Nd"(port));
    return 0;
}

static BauraUart serial_uart;

static int serial_write(const char *text, uint32_t length) {
    return uart_write(&serial_uart, text, length);
}

static int serial_init(void) {
    const BauraUartIo io = {
        .write8 = serial_port_write8,
        .read8 = serial_port_read8,
        .context = 0
    };
    return uart_init(&serial_uart, &io);
}

static __attribute__((noreturn)) void halt(void) {
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

static void serial_write_or_halt(const char *text, uint32_t length) {
    if (serial_write(text, length) != BAURA_UART_OK) halt();
}

void kernel_main(uint32_t magic, uint32_t multiboot_info) {
    static const char boot_message[] = "BAURA-OS booting\r\n";
    static const char magic_error[] = "ERROR: invalid Multiboot magic\r\n";
    static const char core_error[] = "ERROR: BAURA Core initialization failed\r\n";
    static const char ready_message[] =
        "CORE_READY: BAURA Core services initialized\r\n";
    (void)multiboot_info;

    if (serial_init() != BAURA_UART_OK) halt();
    serial_write_or_halt(boot_message, sizeof(boot_message) - 1u);

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        serial_write_or_halt(magic_error, sizeof(magic_error) - 1u);
        halt();
    }

    if (baura_system_start() != BAURA_OK) {
        serial_write_or_halt(core_error, sizeof(core_error) - 1u);
        halt();
    }
    serial_write_or_halt(ready_message, sizeof(ready_message) - 1u);
    halt();
}
