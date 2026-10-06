#include "baura_gui_platform.h"

#include <algorithm>
#include <climits>
#include <limits>
#include <new>

static constexpr size_t MaximumFramebufferPixels = 4194304u;

BauraGuiPlatform::BauraGuiPlatform()
    : target{},
      input{},
      input_ready(false),
      initialized(false),
      running(false),
      pointer_x(0),
      pointer_y(0),
      mouse_buttons(0),
      error(BAURA_GUI_PLATFORM_OK),
      backend_error(0) {}

bool BauraGuiPlatform::initialize(const BauraMappedFramebuffer &framebuffer,
                                  const BauraPs2Io *input_io) {
    backend_error = 0;
    if (initialized || framebuffer.pixels == nullptr ||
        framebuffer.width == 0 || framebuffer.height == 0 ||
        framebuffer.width > static_cast<uint32_t>(INT_MAX) ||
        framebuffer.height > static_cast<uint32_t>(INT_MAX) ||
        framebuffer.width > std::numeric_limits<uint32_t>::max() / 4u ||
        framebuffer.pitch_bytes < framebuffer.width * 4u ||
        framebuffer.height >
            std::numeric_limits<size_t>::max() / framebuffer.pitch_bytes ||
        framebuffer.capacity_bytes <
            static_cast<size_t>(framebuffer.pitch_bytes) * framebuffer.height) {
        error = BAURA_GUI_PLATFORM_ERR_ARGUMENT;
        return false;
    }
    if (framebuffer.width > MaximumFramebufferPixels / framebuffer.height) {
        error = BAURA_GUI_PLATFORM_ERR_ARGUMENT;
        return false;
    }
    const size_t count = static_cast<size_t>(framebuffer.width) *
                         framebuffer.height;
    if (framebuffer.format != BauraFramebufferFormat::RGBX8888 &&
        framebuffer.format != BauraFramebufferFormat::BGRX8888) {
        error = BAURA_GUI_PLATFORM_ERR_ARGUMENT;
        return false;
    }
    target = framebuffer;
    try {
        staging.assign(count, 0xFF101827u);
        renderer.reset(new (std::nothrow) BGERenderer(
            {staging.data(), target.width, target.height, target.width,
             staging.size()}));
        if (!renderer) {
            error = BAURA_GUI_PLATFORM_ERR_RESOURCE;
            return false;
        }
        desktop.reset(new (std::nothrow) BGEUIFramework(*renderer));
        if (!desktop) {
            renderer.reset();
            error = BAURA_GUI_PLATFORM_ERR_RESOURCE;
            return false;
        }
    } catch (const std::bad_alloc &) {
        desktop.reset();
        renderer.reset();
        staging.clear();
        error = BAURA_GUI_PLATFORM_ERR_RESOURCE;
        return false;
    }
    if (input_io != nullptr) {
        const int input_status = baura_ps2_init(&input, input_io);
        if (input_status != BAURA_PS2_OK) {
            desktop.reset();
            renderer.reset();
            staging.clear();
            error = BAURA_GUI_PLATFORM_ERR_INPUT;
            backend_error = input_status;
            return false;
        }
        input_ready = true;
    }
    pointer_x = target.width / 2u;
    pointer_y = target.height / 2u;
    const int display_status = display.set_framebuffer(
        staging.data(), target.width, target.height, target.width,
        staging.size(), &BauraGuiPlatform::present, this);
    if (display_status != DISPLAY_OK) {
        desktop.reset();
        renderer.reset();
        staging.clear();
        error = BAURA_GUI_PLATFORM_ERR_DISPLAY;
        backend_error = display_status;
        return false;
    }
    initialized = true;
    error = BAURA_GUI_PLATFORM_OK;
    backend_error = 0;
    return true;
}

bool BauraGuiPlatform::start() {
    if (!initialized || renderer == nullptr || desktop == nullptr) {
        error = BAURA_GUI_PLATFORM_ERR_STATE;
        return false;
    }
    running = true;
    return render(0.0f);
}

void BauraGuiPlatform::stop() { running = false; }

bool BauraGuiPlatform::render(float animation_phase) {
    if (!running || renderer == nullptr || desktop == nullptr) {
        error = BAURA_GUI_PLATFORM_ERR_STATE;
        return false;
    }
    desktop->render_desktop(animation_phase);
    if (input_ready) {
        const int cursor_right = static_cast<int>(
            std::min(target.width - 1u, pointer_x + 8u));
        const int cursor_bottom = static_cast<int>(
            std::min(target.height - 1u, pointer_y + 12u));
        renderer->draw_line(static_cast<int>(pointer_x),
                            static_cast<int>(pointer_y),
                            static_cast<int>(pointer_x),
                            cursor_bottom, 0xFF101827u);
        renderer->draw_line(static_cast<int>(pointer_x),
                            static_cast<int>(pointer_y),
                            cursor_right,
                            static_cast<int>(std::min(
                                target.height - 1u, pointer_y + 8u)),
                            0xFF101827u);
        renderer->draw_line(static_cast<int>(pointer_x),
                            static_cast<int>(std::min(
                                target.height - 1u, pointer_y + 2u)),
                            static_cast<int>(std::min(
                                target.width - 1u, pointer_x + 2u)),
                            static_cast<int>(std::min(
                                target.height - 1u, pointer_y + 10u)),
                            0xFFFFFFFFu);
    }
    const int status = display.flip_buffer();
    if (status != DISPLAY_OK) {
        error = BAURA_GUI_PLATFORM_ERR_DISPLAY;
        backend_error = status;
        return false;
    }
    error = BAURA_GUI_PLATFORM_OK;
    backend_error = 0;
    return true;
}

int BauraGuiPlatform::poll_input(uint32_t maximum_events) {
    backend_error = 0;
    if (!running) {
        error = BAURA_GUI_PLATFORM_ERR_STATE;
        return error;
    }
    if (!input_ready) {
        error = BAURA_GUI_PLATFORM_ERR_INPUT;
        backend_error = BAURA_PS2_ERR_STATE;
        return error;
    }
    if (maximum_events == 0 || maximum_events > 128) {
        error = BAURA_GUI_PLATFORM_ERR_ARGUMENT;
        return error;
    }
    BauraPs2Event events[128]{};
    uint32_t count = 0;
    const int status =
        baura_ps2_poll(&input, events, maximum_events, &count);
    if (status != BAURA_PS2_OK) {
        error = BAURA_GUI_PLATFORM_ERR_INPUT;
        backend_error = status;
        return error;
    }
    for (uint32_t index = 0; index < count; ++index) {
        handle_event(events[index]);
    }
    if (count != 0 && !render(0.0f)) return error;
    return static_cast<int>(count);
}

void BauraGuiPlatform::set_action_handler(
    void (*handler)(uint32_t, void *), void *context) {
    if (desktop != nullptr) {
        desktop->taskbar().set_action_handler(handler, context);
    }
}

BGEUIFramework *BauraGuiPlatform::shell() { return desktop.get(); }
bool BauraGuiPlatform::ready() const { return running; }
int BauraGuiPlatform::last_error() const { return error; }
int BauraGuiPlatform::last_backend_error() const { return backend_error; }

int BauraGuiPlatform::present(void *context, uint32_t *pixels,
                              uint32_t width, uint32_t height,
                              uint32_t stride_pixels) {
    BauraGuiPlatform *platform = static_cast<BauraGuiPlatform *>(context);
    if (platform == nullptr || pixels == nullptr ||
        width != platform->target.width || height != platform->target.height ||
        stride_pixels < width) return -1;
    for (uint32_t y = 0; y < height; ++y) {
        volatile uint8_t *destination =
            platform->target.pixels +
            static_cast<size_t>(y) * platform->target.pitch_bytes;
        const uint32_t *source =
            pixels + static_cast<size_t>(y) * stride_pixels;
        for (uint32_t x = 0; x < width; ++x) {
            const uint32_t argb = source[x];
            if (platform->target.format == BauraFramebufferFormat::RGBX8888) {
                destination[x * 4u] = static_cast<uint8_t>(argb >> 16);
                destination[x * 4u + 1u] = static_cast<uint8_t>(argb >> 8);
                destination[x * 4u + 2u] = static_cast<uint8_t>(argb);
            } else {
                destination[x * 4u] = static_cast<uint8_t>(argb);
                destination[x * 4u + 1u] = static_cast<uint8_t>(argb >> 8);
                destination[x * 4u + 2u] = static_cast<uint8_t>(argb >> 16);
            }
            destination[x * 4u + 3u] = 0;
        }
    }
    if (platform->target.flush != nullptr &&
        platform->target.flush(platform->target.flush_context) != 0) {
        return -1;
    }
    return 0;
}

void BauraGuiPlatform::handle_event(const BauraPs2Event &event) {
    if (desktop == nullptr) return;
    if (event.type == BAURA_PS2_EVENT_MOUSE) {
        const int64_t next_x = static_cast<int64_t>(pointer_x) +
                               event.data.mouse.delta_x;
        const int64_t next_y = static_cast<int64_t>(pointer_y) +
                               event.data.mouse.delta_y;
        pointer_x = static_cast<uint32_t>(std::max<int64_t>(
            0, std::min<int64_t>(target.width - 1u, next_x)));
        pointer_y = static_cast<uint32_t>(std::max<int64_t>(
            0, std::min<int64_t>(target.height - 1u, next_y)));
        const uint8_t pressed = event.data.mouse.buttons & 1u;
        if (pressed != 0 && (mouse_buttons & 1u) == 0) {
            desktop->process_event(EventType::CLICK,
                                   static_cast<int>(pointer_x),
                                   static_cast<int>(pointer_y));
        }
        if (pressed == 0 && (mouse_buttons & 1u) != 0) {
            desktop->process_event(EventType::RELEASE,
                                   static_cast<int>(pointer_x),
                                   static_cast<int>(pointer_y));
        } else if (pressed != 0) {
            desktop->process_event(EventType::DRAG,
                                   static_cast<int>(pointer_x),
                                   static_cast<int>(pointer_y));
        }
        mouse_buttons = event.data.mouse.buttons;
    } else if (event.type == BAURA_PS2_EVENT_KEY &&
               event.data.key.pressed != 0) {
        BauraTaskbar &taskbar = desktop->taskbar();
        if (taskbar.is_search_open()) {
            if (event.data.key.ascii == '\b') {
                taskbar.handle_search_backspace();
            } else if (event.data.key.ascii >= 0x20) {
                taskbar.handle_search_character(
                    static_cast<char>(event.data.key.ascii));
            }
        }
    }
}
