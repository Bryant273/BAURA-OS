#include "ps2_driver.h"

#include <cstring>

namespace {

constexpr uint8_t StatusOutputFull = 0x01;
constexpr uint8_t StatusInputFull = 0x02;
constexpr uint8_t StatusAuxiliary = 0x20;
constexpr uint8_t StatusError = 0xC0;
constexpr uint8_t CommandEnableKeyboard = 0xAE;
constexpr uint8_t CommandEnableMouse = 0xA8;
constexpr uint8_t CommandWriteMouse = 0xD4;
constexpr uint8_t DeviceEnableScanning = 0xF4;
constexpr uint8_t DeviceAcknowledge = 0xFA;
constexpr uint8_t DeviceResend = 0xFE;

int read_status(BauraPs2Driver *driver, uint8_t *status) {
    return driver->io.read8(driver->io.context, BAURA_PS2_STATUS_PORT, status);
}

int wait_input_empty(BauraPs2Driver *driver) {
    for (uint32_t attempt = 0; attempt < BAURA_PS2_POLL_LIMIT; ++attempt) {
        uint8_t status = 0;
        if (read_status(driver, &status) != 0) return BAURA_PS2_ERR_IO;
        if ((status & StatusInputFull) == 0) return BAURA_PS2_OK;
    }
    return BAURA_PS2_ERR_TIMEOUT;
}

int write_controller(BauraPs2Driver *driver, uint8_t value) {
    const int wait = wait_input_empty(driver);
    if (wait != BAURA_PS2_OK) return wait;
    return driver->io.write8(driver->io.context, BAURA_PS2_DATA_PORT, value) == 0
               ? BAURA_PS2_OK
               : BAURA_PS2_ERR_IO;
}

int write_command(BauraPs2Driver *driver, uint8_t value) {
    const int wait = wait_input_empty(driver);
    if (wait != BAURA_PS2_OK) return wait;
    return driver->io.write8(driver->io.context, BAURA_PS2_STATUS_PORT, value) == 0
               ? BAURA_PS2_OK
               : BAURA_PS2_ERR_IO;
}

int read_response(BauraPs2Driver *driver, uint8_t *value) {
    for (uint32_t attempt = 0; attempt < BAURA_PS2_POLL_LIMIT; ++attempt) {
        uint8_t status = 0;
        if (read_status(driver, &status) != 0) return BAURA_PS2_ERR_IO;
        if ((status & StatusOutputFull) == 0) continue;
        if ((status & StatusError) != 0) return BAURA_PS2_ERR_IO;
        if (driver->io.read8(driver->io.context, BAURA_PS2_DATA_PORT,
                             value) != 0) return BAURA_PS2_ERR_IO;
        return BAURA_PS2_OK;
    }
    return BAURA_PS2_ERR_TIMEOUT;
}

int send_device_command(BauraPs2Driver *driver, bool mouse, uint8_t command) {
    for (unsigned retry = 0; retry < 3; ++retry) {
        if (mouse) {
            const int prefix = write_command(driver, CommandWriteMouse);
            if (prefix != BAURA_PS2_OK) return prefix;
        }
        const int write = write_controller(driver, command);
        if (write != BAURA_PS2_OK) return write;
        uint8_t response = 0;
        const int read = read_response(driver, &response);
        if (read != BAURA_PS2_OK) return read;
        if (response == DeviceAcknowledge) return BAURA_PS2_OK;
        if (response != DeviceResend) return BAURA_PS2_ERR_DEVICE;
    }
    return BAURA_PS2_ERR_DEVICE;
}

uint8_t translate_key(uint16_t scan_code, bool shift, bool caps_lock) {
    const bool letter_case = shift != caps_lock;
    switch (scan_code) {
    case 0x1C: return letter_case ? 'A' : 'a';
    case 0x32: return letter_case ? 'B' : 'b';
    case 0x21: return letter_case ? 'C' : 'c';
    case 0x23: return letter_case ? 'D' : 'd';
    case 0x24: return letter_case ? 'E' : 'e';
    case 0x2B: return letter_case ? 'F' : 'f';
    case 0x34: return letter_case ? 'G' : 'g';
    case 0x33: return letter_case ? 'H' : 'h';
    case 0x43: return letter_case ? 'I' : 'i';
    case 0x3B: return letter_case ? 'J' : 'j';
    case 0x42: return letter_case ? 'K' : 'k';
    case 0x4B: return letter_case ? 'L' : 'l';
    case 0x3A: return letter_case ? 'M' : 'm';
    case 0x31: return letter_case ? 'N' : 'n';
    case 0x44: return letter_case ? 'O' : 'o';
    case 0x4D: return letter_case ? 'P' : 'p';
    case 0x15: return letter_case ? 'Q' : 'q';
    case 0x2D: return letter_case ? 'R' : 'r';
    case 0x1B: return letter_case ? 'S' : 's';
    case 0x2C: return letter_case ? 'T' : 't';
    case 0x3C: return letter_case ? 'U' : 'u';
    case 0x2A: return letter_case ? 'V' : 'v';
    case 0x1D: return letter_case ? 'W' : 'w';
    case 0x22: return letter_case ? 'X' : 'x';
    case 0x35: return letter_case ? 'Y' : 'y';
    case 0x1A: return letter_case ? 'Z' : 'z';
    case 0x29: return ' ';
    case 0x5A: return '\n';
    case 0x66: return '\b';
    case 0x4E: return shift ? '_' : '-';
    case 0x55: return shift ? '+' : '=';
    case 0x54: return shift ? '{' : '[';
    case 0x5B: return shift ? '}' : ']';
    case 0x4C: return shift ? ':' : ';';
    case 0x52: return shift ? '"' : '\'';
    case 0x41: return shift ? '<' : ',';
    case 0x49: return shift ? '>' : '.';
    case 0x4A: return shift ? '?' : '/';
    case 0x16: return shift ? '!' : '1';
    case 0x1E: return shift ? '@' : '2';
    case 0x26: return shift ? '#' : '3';
    case 0x25: return shift ? '$' : '4';
    case 0x2E: return shift ? '%' : '5';
    case 0x36: return shift ? '^' : '6';
    case 0x3D: return shift ? '&' : '7';
    case 0x3E: return shift ? '*' : '8';
    case 0x46: return shift ? '(' : '9';
    case 0x45: return shift ? ')' : '0';
    default: return 0;
    }
}

bool decode_keyboard(BauraPs2Driver *driver, uint8_t raw,
                     BauraPs2Event *event) {
    if (raw == 0xE0) {
        driver->extended = 1;
        return false;
    }
    if (raw == 0xF0) {
        driver->key_released = 1;
        return false;
    }
    const uint8_t released = driver->key_released;
    const uint8_t extended = driver->extended;
    const uint16_t code = static_cast<uint16_t>(raw) |
                          (extended ? 0x100u : 0u);
    driver->extended = 0;
    driver->key_released = 0;
    if (code == 0x12) driver->left_shift = !released;
    if (code == 0x59) driver->right_shift = !released;
    if (code == 0x58 && !released) driver->caps_lock ^= 1u;

    event->type = BAURA_PS2_EVENT_KEY;
    event->data.key.scan_code = code;
    event->data.key.pressed = released ? 0u : 1u;
    event->data.key.modifiers =
        static_cast<uint8_t>((driver->left_shift || driver->right_shift ? 1u : 0u) |
                             (driver->caps_lock ? 2u : 0u));
    event->data.key.ascii =
        released || extended
            ? 0
            : translate_key(raw, driver->left_shift || driver->right_shift,
                            driver->caps_lock != 0);
    return true;
}

bool decode_mouse(BauraPs2Driver *driver, uint8_t raw,
                  BauraPs2Event *event) {
    if (driver->mouse_packet_length == 0 && (raw & 0x08u) == 0) return false;
    driver->mouse_packet[driver->mouse_packet_length++] = raw;
    if (driver->mouse_packet_length < 3) return false;
    driver->mouse_packet_length = 0;
    const uint8_t flags = driver->mouse_packet[0];
    if ((flags & 0xC0u) != 0) return false;
    int16_t delta_x = driver->mouse_packet[1];
    int16_t delta_y = driver->mouse_packet[2];
    if ((flags & 0x10u) != 0) delta_x -= 256;
    if ((flags & 0x20u) != 0) delta_y -= 256;
    event->type = BAURA_PS2_EVENT_MOUSE;
    event->data.mouse.delta_x = delta_x;
    event->data.mouse.delta_y = static_cast<int16_t>(-delta_y);
    event->data.mouse.buttons = flags & 0x07u;
    return true;
}

}

extern "C" int baura_ps2_init(BauraPs2Driver *driver,
                               const BauraPs2Io *io) {
    if (driver == nullptr || io == nullptr || io->read8 == nullptr ||
        io->write8 == nullptr) return BAURA_PS2_ERR_ARGUMENT;
    std::memset(driver, 0, sizeof(*driver));
    driver->io = *io;
    int status = write_command(driver, CommandEnableKeyboard);
    if (status != BAURA_PS2_OK) return status;
    status = write_command(driver, CommandEnableMouse);
    if (status != BAURA_PS2_OK) return status;
    status = send_device_command(driver, false, DeviceEnableScanning);
    if (status != BAURA_PS2_OK) return status;
    status = send_device_command(driver, true, DeviceEnableScanning);
    if (status != BAURA_PS2_OK) return status;
    driver->initialized = 1;
    return BAURA_PS2_OK;
}

extern "C" int baura_ps2_poll(BauraPs2Driver *driver, BauraPs2Event *events,
                               uint32_t capacity, uint32_t *event_count) {
    if (driver == nullptr || events == nullptr || event_count == nullptr ||
        capacity == 0) return BAURA_PS2_ERR_ARGUMENT;
    *event_count = 0;
    if (!driver->initialized) return BAURA_PS2_ERR_STATE;
    while (*event_count < capacity) {
        uint8_t status = 0;
        if (read_status(driver, &status) != 0) return BAURA_PS2_ERR_IO;
        if ((status & StatusOutputFull) == 0) break;
        if ((status & StatusError) != 0) return BAURA_PS2_ERR_IO;
        uint8_t raw = 0;
        if (driver->io.read8(driver->io.context, BAURA_PS2_DATA_PORT, &raw) != 0) {
            return BAURA_PS2_ERR_IO;
        }
        BauraPs2Event event{};
        const bool produced = (status & StatusAuxiliary) != 0
                                  ? decode_mouse(driver, raw, &event)
                                  : decode_keyboard(driver, raw, &event);
        if (produced) events[(*event_count)++] = event;
    }
    return BAURA_PS2_OK;
}
