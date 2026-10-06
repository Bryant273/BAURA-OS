#ifndef BAURA_GUI_PLATFORM_H
#define BAURA_GUI_PLATFORM_H

#include "../BGE/ui_framework.h"
#include "../INPUT/ps2_driver.h"
#include "../DISPLAY_SERVER/BAURA_DISPLAY_SERVER.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

enum class BauraFramebufferFormat {
    RGBX8888,
    BGRX8888
};

typedef int (*BauraFramebufferFlush)(void *context);

struct BauraMappedFramebuffer {
    volatile uint8_t *pixels;
    size_t capacity_bytes;
    uint32_t width;
    uint32_t height;
    uint32_t pitch_bytes;
    BauraFramebufferFormat format;
    BauraFramebufferFlush flush;
    void *flush_context;
};

enum BauraGuiPlatformError {
    BAURA_GUI_PLATFORM_OK = 0,
    BAURA_GUI_PLATFORM_ERR_ARGUMENT = -100,
    BAURA_GUI_PLATFORM_ERR_RESOURCE = -101,
    BAURA_GUI_PLATFORM_ERR_INPUT = -102,
    BAURA_GUI_PLATFORM_ERR_DISPLAY = -103,
    BAURA_GUI_PLATFORM_ERR_STATE = -104
};

class BauraGuiPlatform {
public:
    BauraGuiPlatform();
    bool initialize(const BauraMappedFramebuffer &framebuffer,
                    const BauraPs2Io *input_io = nullptr);
    bool start();
    void stop();
    bool render(float animation_phase);
    int poll_input(uint32_t maximum_events = 32);
    void set_action_handler(void (*handler)(uint32_t, void *), void *context);
    BGEUIFramework *shell();
    bool ready() const;
    int last_error() const;
    int last_backend_error() const;

private:
    static int present(void *context, uint32_t *pixels, uint32_t width,
                       uint32_t height, uint32_t stride_pixels);
    void handle_event(const BauraPs2Event &event);
    BauraMappedFramebuffer target;
    std::vector<uint32_t> staging;
    std::unique_ptr<BGERenderer> renderer;
    std::unique_ptr<BGEUIFramework> desktop;
    DisplayServer display;
    BauraPs2Driver input;
    bool input_ready;
    bool initialized;
    bool running;
    uint32_t pointer_x;
    uint32_t pointer_y;
    uint8_t mouse_buttons;
    int error;
    int backend_error;
};

#endif
