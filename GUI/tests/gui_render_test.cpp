#include <cassert>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <limits>

#include "../BAURA_GUI_FFI.h"
#include "../BGE/effects_engine.h"
#include "../BGE/gpu_accel_3d.h"
#include "../BGE/ui_framework.h"
#include "../INPUT/ps2_driver.h"
#include "../PLATFORM/baura_gui_core_service.h"
#include "../PLATFORM/baura_gui_platform.h"
#include "../../KERNEL/BAURA_CORE/SERVICE_MANAGER/baura_service_manager.h"
#include "../DESKTOP/baura_desktop.h"
#include "../LOGIN_SCREEN/login_screen.h"
#include "../UI_FRAMEWORK/BAURA_UI_FRAMEWORK.h"
#include "../UI_FRAMEWORK/baura_brand.h"
#include "../UI_FRAMEWORK/baura_hybrid_taskbar.h"
#include "../UI_FRAMEWORK/papirus_icon_catalog.h"
#include "../UI_FRAMEWORK/system_bubble.h"
#include "../UI_FRAMEWORK/ui_extensions.h"
#include "../WINDOW_MANAGER/BAURA_WINDOW_MANAGER.h"
#include "../WINDOW_MANAGER/input_handler.h"
#include "../../BOOT/INSTALLER/installer_gui.h"

static uint32_t input_calls;
static uint32_t overlay_requests;
static uint32_t window_actions;
static BauraWindowAction last_window_action;
static uint32_t installer_actions;
static uint32_t taskbar_actions[12];

struct MockPs2 {
    uint8_t values[64];
    uint8_t auxiliary[64];
    uint32_t read_index;
    uint32_t write_index;
    bool next_device_is_mouse;
};

static void mock_ps2_push(MockPs2 *mock, uint8_t value, bool auxiliary) {
    assert(mock->write_index < 64);
    mock->values[mock->write_index] = value;
    mock->auxiliary[mock->write_index] = auxiliary ? 1u : 0u;
    ++mock->write_index;
}

static int mock_ps2_read(void *context, uint16_t port, uint8_t *value) {
    MockPs2 *mock = static_cast<MockPs2 *>(context);
    if (port == BAURA_PS2_STATUS_PORT) {
        if (mock->read_index == mock->write_index) {
            *value = 0;
        } else {
            *value = static_cast<uint8_t>(
                0x01u | (mock->auxiliary[mock->read_index] ? 0x20u : 0u));
        }
        return 0;
    }
    if (port != BAURA_PS2_DATA_PORT ||
        mock->read_index == mock->write_index) return -1;
    *value = mock->values[mock->read_index++];
    return 0;
}

static int mock_ps2_write(void *context, uint16_t port, uint8_t value) {
    MockPs2 *mock = static_cast<MockPs2 *>(context);
    if (port == BAURA_PS2_STATUS_PORT && value == 0xD4) {
        mock->next_device_is_mouse = true;
        return 0;
    }
    if (port == BAURA_PS2_DATA_PORT && value == 0xF4) {
        mock_ps2_push(mock, 0xFA, mock->next_device_is_mouse);
        mock->next_device_is_mouse = false;
    }
    return 0;
}

static int flush_framebuffer(void *context) {
    ++*static_cast<uint32_t *>(context);
    return 0;
}

static int fail_framebuffer_flush(void *) { return -1; }

static void test_ps2_driver() {
    MockPs2 mock{};
    BauraPs2Io io{mock_ps2_read, mock_ps2_write, &mock};
    BauraPs2Driver driver{};
    assert(baura_ps2_init(&driver, &io) == BAURA_PS2_OK);
    assert(driver.initialized == 1);
    mock_ps2_push(&mock, 0x1C, false);
    mock_ps2_push(&mock, 0xF0, false);
    mock_ps2_push(&mock, 0x1C, false);
    mock_ps2_push(&mock, 0x09, true);
    mock_ps2_push(&mock, 0x05, true);
    mock_ps2_push(&mock, 0x00, true);
    BauraPs2Event events[4]{};
    uint32_t count = 0;
    assert(baura_ps2_poll(&driver, events, 4, &count) == BAURA_PS2_OK);
    assert(count == 3);
    assert(events[0].type == BAURA_PS2_EVENT_KEY);
    assert(events[0].data.key.pressed == 1 &&
           events[0].data.key.ascii == 'a');
    assert(events[1].data.key.pressed == 0);
    assert(events[2].type == BAURA_PS2_EVENT_MOUSE);
    assert(events[2].data.mouse.delta_x == 5 &&
           events[2].data.mouse.buttons == 1);
    assert(baura_ps2_poll(&driver, events, 0, &count) ==
           BAURA_PS2_ERR_ARGUMENT);
}

static void test_gui_platform_service() {
    uint8_t framebuffer_rgb[8 * 4 * 4];
    uint8_t framebuffer_bgr[8 * 4 * 4];
    for (uint8_t &byte : framebuffer_rgb) byte = 0xA5;
    for (uint8_t &byte : framebuffer_bgr) byte = 0xA5;
    uint32_t flushes = 0;
    BauraMappedFramebuffer rgb{
        framebuffer_rgb, sizeof(framebuffer_rgb), 8, 4, 8 * 4,
        BauraFramebufferFormat::RGBX8888, flush_framebuffer, &flushes};
    BauraMappedFramebuffer bgr{
        framebuffer_bgr, sizeof(framebuffer_bgr), 8, 4, 8 * 4,
        BauraFramebufferFormat::BGRX8888, nullptr, nullptr};
    BauraMappedFramebuffer invalid = rgb;
    invalid.pitch_bytes = 4;
    BauraGuiPlatform rejected_pitch;
    assert(!rejected_pitch.initialize(invalid));
    assert(rejected_pitch.last_error() == BAURA_GUI_PLATFORM_ERR_ARGUMENT);
    invalid = rgb;
    invalid.capacity_bytes--;
    BauraGuiPlatform rejected_capacity;
    assert(!rejected_capacity.initialize(invalid));
    assert(rejected_capacity.last_error() == BAURA_GUI_PLATFORM_ERR_ARGUMENT);
    BauraGuiPlatform first;
    BauraGuiPlatform second;
    MockPs2 platform_input{};
    MockPs2 bgr_input{};
    BauraPs2Io platform_io{
        mock_ps2_read, mock_ps2_write, &platform_input};
    BauraPs2Io bgr_io{mock_ps2_read, mock_ps2_write, &bgr_input};
    assert(first.initialize(rgb, &platform_io));
    assert(second.initialize(bgr, &bgr_io));
    service_manager_init();
    BauraGuiServiceBinding binding{&first, 0};
    assert(baura_gui_core_service_start(&binding) == BAURA_OK);
    assert(first.ready() && flushes == 1);
    BauraServiceInfo service{};
    assert(baura_service_query(binding.service_id, &service) == BAURA_OK);
    assert(service.state == BAURA_SERVICE_RUNNING);
    assert(second.start());
    for (uint32_t y = 0; y < 4; ++y) {
        for (uint32_t x = 0; x < 8; ++x) {
            const size_t offset = static_cast<size_t>(y) * 8 * 4 + x * 4;
            assert(framebuffer_rgb[offset] == framebuffer_bgr[offset + 2]);
            assert(framebuffer_rgb[offset + 1] == framebuffer_bgr[offset + 1]);
            assert(framebuffer_rgb[offset + 2] == framebuffer_bgr[offset]);
            assert(framebuffer_rgb[offset + 3] == 0);
        }
    }
    mock_ps2_push(&platform_input, 0x08, true);
    mock_ps2_push(&platform_input, 0x01, true);
    mock_ps2_push(&platform_input, 0x00, true);
    assert(first.poll_input() == 1);
    assert(baura_gui_core_service_stop(&binding) == BAURA_OK);
    assert(!first.ready());
    assert(baura_service_query(binding.service_id, &service) == BAURA_OK);
    assert(service.state == BAURA_SERVICE_STOPPED);

    invalid = rgb;
    invalid.format = static_cast<BauraFramebufferFormat>(99);
    BauraGuiPlatform rejected;
    assert(!rejected.initialize(invalid));
    assert(rejected.last_error() == BAURA_GUI_PLATFORM_ERR_ARGUMENT);
    BauraMappedFramebuffer failing = rgb;
    failing.flush = fail_framebuffer_flush;
    BauraGuiPlatform flush_failure;
    assert(flush_failure.initialize(failing));
    assert(!flush_failure.start());
    assert(flush_failure.last_error() == BAURA_GUI_PLATFORM_ERR_DISPLAY);
    assert(flush_failure.last_backend_error() == DISPLAY_ERR_UNAVAILABLE);
}

static void window_input(uint32_t window_id, BauraWindowInputEvent event,
                         int x, int y, void *context) {
    uint32_t *calls = static_cast<uint32_t *>(context);
    assert(window_id != 0 && event == BauraWindowInputEvent::CLICK);
    assert(x == 12 && y == 34);
    ++*calls;
}

static int request_security_overlay(void *context) {
    uint32_t *calls = static_cast<uint32_t *>(context);
    ++*calls;
    return 0;
}

static void window_action(BauraWindowAction action, void *context) {
    uint32_t *calls = static_cast<uint32_t *>(context);
    last_window_action = action;
    ++*calls;
}

static int installer_action(void *context, BauraInstallStage stage) {
    uint32_t *calls = static_cast<uint32_t *>(context);
    assert(stage >= BauraInstallStage::HardwareScan &&
           stage <= BauraInstallStage::CopySystem);
    ++*calls;
    return 0;
}

static int present(void *context, uint32_t *pixels, uint32_t width,
                   uint32_t height, uint32_t stride) {
    uint32_t *calls = static_cast<uint32_t *>(context);
    assert(pixels != nullptr && width == 4 && height == 3 && stride == 5);
    ++*calls;
    return 0;
}

static int fail_present(void *, uint32_t *, uint32_t, uint32_t, uint32_t) {
    return -1;
}

static void taskbar_action(uint32_t action, void *context) {
    uint32_t *calls = static_cast<uint32_t *>(context);
    assert(action >= BauraTaskbar::ActionStartMenu &&
           action <= BauraTaskbar::ActionActivateApp);
    ++calls[action];
}

static bool write_bmp(const char *path, const BGEFramebuffer &framebuffer) {
    if (path == nullptr || path[0] == '\0' || framebuffer.pixels == nullptr ||
        framebuffer.width == 0 || framebuffer.height == 0 ||
        framebuffer.width > static_cast<uint32_t>(INT_MAX) ||
        framebuffer.height > static_cast<uint32_t>(INT_MAX) ||
        framebuffer.stride_pixels < framebuffer.width) return false;

    const uint64_t row_bytes =
        (static_cast<uint64_t>(framebuffer.width) * 3u + 3u) & ~3ull;
    if (row_bytes > std::numeric_limits<uint64_t>::max() /
                        framebuffer.height) return false;
    const uint64_t image_bytes = row_bytes * framebuffer.height;
    if (image_bytes > std::numeric_limits<uint32_t>::max() - 54u) {
        return false;
    }
    const uint64_t file_bytes = 54u + image_bytes;

    std::ofstream output(path, std::ios::binary);
    if (!output) return false;
    const auto write_u16 = [&output](uint16_t value) {
        const char bytes[] = {
            static_cast<char>(value),
            static_cast<char>(value >> 8)
        };
        output.write(bytes, sizeof(bytes));
    };
    const auto write_u32 = [&output](uint32_t value) {
        const char bytes[] = {
            static_cast<char>(value),
            static_cast<char>(value >> 8),
            static_cast<char>(value >> 16),
            static_cast<char>(value >> 24)
        };
        output.write(bytes, sizeof(bytes));
    };

    output.put('B');
    output.put('M');
    write_u32(static_cast<uint32_t>(file_bytes));
    write_u16(0);
    write_u16(0);
    write_u32(54);
    write_u32(40);
    write_u32(framebuffer.width);
    write_u32(framebuffer.height);
    write_u16(1);
    write_u16(24);
    write_u32(0);
    write_u32(static_cast<uint32_t>(image_bytes));
    write_u32(2835);
    write_u32(2835);
    write_u32(0);
    write_u32(0);

    const char padding[3] = {};
    const size_t padding_bytes =
        static_cast<size_t>(row_bytes) -
        static_cast<size_t>(framebuffer.width) * 3u;
    for (uint32_t row = framebuffer.height; row > 0; --row) {
        const uint32_t *pixels =
            framebuffer.pixels +
            static_cast<size_t>(row - 1u) * framebuffer.stride_pixels;
        for (uint32_t column = 0; column < framebuffer.width; ++column) {
            const uint32_t pixel = pixels[column];
            const char bgr[] = {
                static_cast<char>(pixel),
                static_cast<char>(pixel >> 8),
                static_cast<char>(pixel >> 16)
            };
            output.write(bgr, sizeof(bgr));
        }
        output.write(padding, static_cast<std::streamsize>(padding_bytes));
    }
    output.close();
    return !output.fail();
}

int main() {
    test_ps2_driver();
    test_gui_platform_service();
    uint32_t framebuffer[15];
    for (uint32_t &pixel : framebuffer) pixel = 0xDEADBEEFu;
    uint32_t presents = 0;
    DisplayServerHandle display = gui_create_display_server();
    assert(display != nullptr);
    assert(gui_compose(nullptr) == BAURA_GUI_ERR_ARGUMENT);
    assert(gui_configure_framebuffer(display, framebuffer, 4, 3, 5, 14,
                                     present, &presents) ==
           BAURA_GUI_ERR_ARGUMENT);
    assert(gui_configure_framebuffer(display, framebuffer, 4, 3, 5, 15,
                                     present, &presents) == BAURA_GUI_OK);
    uint32_t display_width = 0;
    uint32_t display_height = 0;
    assert(gui_get_framebuffer_size(display, &display_width, &display_height) ==
           BAURA_GUI_OK);
    assert(display_width == 4 && display_height == 3);
    assert(gui_get_framebuffer_size(display, nullptr, &display_height) ==
           BAURA_GUI_ERR_ARGUMENT);

    const uint32_t red_pixels[4] = {
        0xFFFF0000u, 0xFFFF0000u, 0xFFFF0000u, 0xFFFF0000u
    };
    const uint32_t blue_pixels[1] = {0xFF0000FFu};
    const uint32_t red = gui_create_surface(display, 2, 2);
    const uint32_t blue = gui_create_surface(display, 1, 1);
    assert(red != 0 && blue != 0);
    assert(gui_update_surface(display, red, red_pixels, 3) ==
           BAURA_GUI_ERR_ARGUMENT);
    assert(gui_update_surface(display, red, red_pixels, 4) == BAURA_GUI_OK);
    assert(gui_update_surface(display, blue, blue_pixels, 1) == BAURA_GUI_OK);

    CompositorHandle compositor = gui_create_compositor(display);
    assert(compositor != nullptr);
    const uint32_t red_window = gui_add_window(compositor, red, -1, 0);
    const uint32_t blue_window = gui_add_window(compositor, blue, 0, 0);
    assert(red_window != 0 && blue_window != 0);
    assert(gui_compose(compositor) == BAURA_GUI_OK);
    assert(framebuffer[0] == 0xFF0000FFu);
    assert(framebuffer[1] == 0xFF000000u);
    assert(framebuffer[5] == 0xFFFF0000u);
    assert(framebuffer[4] == 0xDEADBEEFu);
    assert(presents == 1);

    assert(gui_set_window_z_order(compositor, red_window, 2) == BAURA_GUI_OK);
    assert(gui_resize_surface(display, red, 1, 2) == BAURA_GUI_OK);
    assert(gui_resize_surface(display, red, 8192, 8192) ==
           BAURA_GUI_ERR_CAPACITY);
    assert(gui_move_window(compositor, red_window, 0, 0) == BAURA_GUI_OK);
    assert(gui_compose(compositor) == BAURA_GUI_OK);
    assert(framebuffer[0] == 0xFFFF0000u);
    assert(gui_set_window_visible(compositor, red_window, 0) == BAURA_GUI_OK);
    assert(gui_compose(compositor) == BAURA_GUI_OK);
    assert(framebuffer[0] == 0xFF0000FFu);
    assert(gui_set_window_visible(compositor, red_window, 2) ==
           BAURA_GUI_ERR_ARGUMENT);
    const uint32_t translucent_blue[1] = {0x800000FFu};
    assert(gui_update_surface(display, blue, translucent_blue, 1) ==
           BAURA_GUI_OK);
    assert(gui_resize_surface(display, blue, 1, 2) == BAURA_GUI_OK);
    const uint32_t resized_blue[2] = {0x800000FFu, 0xFF00FF00u};
    assert(gui_update_surface(display, blue, resized_blue, 2) == BAURA_GUI_OK);
    assert(gui_compose(compositor) == BAURA_GUI_OK);
    assert(framebuffer[0] == 0xFF000080u);
    assert(gui_configure_framebuffer(display, framebuffer, 4, 3, 5, 15,
                                     fail_present, nullptr) == BAURA_GUI_OK);
    assert(gui_compose(compositor) == BAURA_GUI_ERR_UNAVAILABLE);
    assert(gui_configure_framebuffer(display, framebuffer, 4, 3, 5, 15,
                                     present, &presents) == BAURA_GUI_OK);

    static uint32_t manager_pixels[640 * 480]{};
    DisplayServer manager_display;
    assert(manager_display.set_framebuffer(manager_pixels, 640, 480, 640,
                                            640 * 480, nullptr, nullptr) ==
           DISPLAY_OK);
    WindowCompositor compositor_object(&manager_display);
    WindowManager manager(&manager_display,
                          &compositor_object);
    const uint32_t managed_window = manager.create_window("test", 200, 150);
    assert(managed_window != 0);
    assert(manager.move_window(managed_window, 1, 1) == DISPLAY_OK);
    assert(manager.resize_window(managed_window, 120, 80) == DISPLAY_OK);
    assert(manager.set_focus(managed_window) == DISPLAY_OK);
    assert(manager.set_input_handler(managed_window, window_input,
                                     &input_calls) == DISPLAY_OK);
    manager.process_input(12, 34, BauraWindowInputEvent::CLICK);
    assert(input_calls == 1);
    assert(manager.set_state(managed_window, WindowState::MAXIMIZED) ==
           DISPLAY_OK);
    assert(manager.state(managed_window) == WindowState::MAXIMIZED);
    assert(manager.restore_window(managed_window) == DISPLAY_OK);
    assert(manager.state(managed_window) == WindowState::NORMAL);
    assert(manager.resize_window(managed_window, 100, 60) == DISPLAY_OK);
    assert(manager.state(managed_window) == WindowState::NORMAL);
    assert(manager.set_state(managed_window, WindowState::MINIMIZED) ==
           DISPLAY_OK);
    assert(manager.state(managed_window) == WindowState::MINIMIZED);
    assert(manager.restore_window(managed_window) == DISPLAY_OK);
    assert(manager.close_window(managed_window) == DISPLAY_OK);
    assert(manager.close_window(managed_window) == DISPLAY_ERR_NOT_FOUND);
    assert(handle_global_shortcuts(0x3, request_security_overlay,
                                   &overlay_requests) ==
           BAURA_SHORTCUT_NOT_HANDLED);
    assert(handle_global_shortcuts(0x7, nullptr, &overlay_requests) ==
           BAURA_SHORTCUT_ERR_UNAVAILABLE);
    assert(handle_global_shortcuts(0x7, request_security_overlay,
                                   &overlay_requests) == 0);
    assert(overlay_requests == 1);

    assert(gui_remove_window(compositor, red_window) == BAURA_GUI_OK);
    assert(gui_remove_window(compositor, blue_window) == BAURA_GUI_OK);
    gui_destroy_display_server(display);
    assert(gui_compose(compositor) == BAURA_GUI_OK);
    gui_destroy_compositor(compositor);

    static uint32_t desktop_pixels[800 * 600];
    BGEFramebuffer desktop_fb{desktop_pixels, 800, 600, 800,
                              sizeof(desktop_pixels) / sizeof(desktop_pixels[0])};
    BGERenderer renderer(desktop_fb);
    assert(renderer.valid());
    renderer.clear(0xFF000000u);
    BauraBrand::draw_mark(renderer, 10, 10, 56);
    assert(renderer.pixel(38, 38) != 0xFF000000u);
    renderer.clear(0xFF000000u);
    BauraBrand::draw_boot_splash(renderer);
    assert(renderer.pixel(0, 0) == 0xFF101827u);
    assert(renderer.pixel(400, 300) != 0xFF101827u);
    renderer.clear(0xFF000000u);
    renderer.draw_pixel_alpha(1, 1, 0xFFFF0000u, 128);
    assert(renderer.pixel(1, 1) == 0xFF800000u);
    renderer.draw_rounded_rect(100, 100, 40, 40, 12, 0xFFFF0000u);
    assert(renderer.pixel(100, 100) == 0xFF000000u);
    assert(renderer.pixel(120, 120) == 0xFFFF0000u);
    renderer.draw_line(std::numeric_limits<int>::min(), 200,
                       std::numeric_limits<int>::max(), 200, 0xFF00FF00u);
    assert(renderer.pixel(0, 200) == 0xFF00FF00u);
    assert(renderer.pixel(799, 200) == 0xFF00FF00u);
    renderer.draw_gradient_rect(-5, -5, 20, 20, 0xFF000000u, 0xFFFFFFFFu);
    assert(renderer.pixel(0, 0) != 0);
    BGEEffects effects(renderer);
    assert(effects.draw_glass_panel(20, 20, 100, 80, 12,
                                    0xFF223344u, 160));
    assert(!effects.apply_blur(0, 0, 10, 10, -1));
    BGE3DRenderer renderer_3d(renderer, 64, 64);
    renderer_3d.clear_depth();
    const BGETriangle far_triangle{
        {4, 4, 0.8f, 0xFFFF0000u},
        {52, 4, 0.8f, 0xFFFF0000u},
        {4, 52, 0.8f, 0xFFFF0000u}
    };
    assert(renderer_3d.draw_triangle(far_triangle));
    const BGETriangle near_triangle{
        {4, 4, 0.2f, 0xFF00FF00u},
        {52, 4, 0.2f, 0xFF00FF00u},
        {4, 52, 0.2f, 0xFF00FF00u}
    };
    assert(renderer_3d.draw_triangle(near_triangle));
    assert(renderer.pixel(10, 10) == 0xFF00FF00u);
    renderer_3d.clear_depth();
    const BGETriangle color_triangle{
        {4, 4, 0.4f, 0xFFFF0000u},
        {52, 4, 0.4f, 0xFF00FF00u},
        {4, 52, 0.4f, 0xFF0000FFu}
    };
    assert(renderer_3d.draw_triangle(color_triangle));
    assert(renderer.pixel(10, 10) == 0xFFBA2323u);
    const BGETriangle invalid_triangle{
        {std::numeric_limits<float>::quiet_NaN(), 4, 0.5f, 0xFFFFFFFFu},
        {10, 4, 0.5f, 0xFFFFFFFFu},
        {4, 10, 0.5f, 0xFFFFFFFFu}
    };
    assert(!renderer_3d.draw_triangle(invalid_triangle));
    const float outside = std::numeric_limits<float>::max();
    const BGETriangle outside_triangle{
        {outside, 4, 0.5f, 0xFFFFFFFFu},
        {outside, 10, 0.5f, 0xFFFFFFFFu},
        {outside, 20, 0.5f, 0xFFFFFFFFu}
    };
    assert(renderer_3d.draw_triangle(outside_triangle));

    BGEWindowManager bge_windows(renderer);
    const uint32_t bge_window =
        bge_windows.create_window("BGE window", {120, 120, 320, 220});
    assert(bge_window != 0);
    bge_windows.composite();
    assert(bge_windows.window_at(130, 130) == bge_window);
    assert(bge_windows.handle_click(120 + 320 - 84 + 32, 130));
    assert(bge_windows.toggle_maximize(bge_window));
    assert(bge_windows.restore(bge_window));
    assert(bge_windows.minimize(bge_window));
    assert(bge_windows.window_at(130, 130) == 0);
    assert(bge_windows.restore(bge_window));
    assert(bge_windows.close_window(bge_window));
    BGEUIFramework bge_ui(renderer);
    const uint32_t shell_window =
        bge_ui.create_window("Desktop app", {100, 100, 320, 220});
    assert(shell_window != 0);
    bge_ui.render_desktop(0.5f);
    assert(bge_ui.handle_pointer(BGEPointerEvent::PRESS, 120, 120));
    assert(bge_ui.handle_pointer(BGEPointerEvent::MOVE, 150, 160));
    assert(bge_ui.handle_pointer(BGEPointerEvent::RELEASE, 150, 160));
    assert(bge_ui.windows().window_at(140, 150) == shell_window);
    assert(bge_ui.handle_click(40, 560));

    size_t icon_count = 0;
    assert(baura_icon_catalog(&icon_count) != nullptr && icon_count >= 20);
    assert(baura_icon_find("folder-home") != nullptr);
    assert(baura_icon_find("unknown") == nullptr);

    BauraTaskbar taskbar;
    BauraTaskbarStatus task_status{true, true, 180, true, "FR",
                                   "22:14", 42, true};
    assert(taskbar.set_status(task_status));
    taskbar.set_notification_count(3);
    assert(taskbar.set_media({"Song", "Artist", true, true}));
    taskbar.set_action_handler(taskbar_action, taskbar_actions);
    assert(taskbar.handle_click(40, 560, 800, 600));
    assert(taskbar_actions[BauraTaskbar::ActionStartMenu] == 1);
    assert(taskbar.handle_click(60, 380, 800, 600));
    assert(taskbar_actions[BauraTaskbar::ActionLaunchFiles] == 1);
    assert(taskbar.handle_click(80, 560, 800, 600));
    assert(taskbar_actions[BauraTaskbar::ActionSearch] == 1);
    assert(taskbar.handle_search_character('b'));
    assert(taskbar.search_query() == "b");
    assert(taskbar.handle_search_backspace());
    assert(taskbar.search_query().empty());
    assert(taskbar_actions[BauraTaskbar::ActionSearchQueryChanged] == 2);
    assert(taskbar.handle_click(540, 20, 800, 600));
    assert(taskbar_actions[BauraTaskbar::ActionNotifications] == 1);
    assert(taskbar.handle_click(420, 20, 800, 600));
    assert(taskbar_actions[BauraTaskbar::ActionOptimize] == 1);
    assert(taskbar.handle_click(650, 230, 800, 600));
    assert(taskbar_actions[BauraTaskbar::ActionOptimize] == 2);
    assert(taskbar.handle_click(600, 20, 800, 600));
    assert(taskbar_actions[BauraTaskbar::ActionMediaToggle] == 1);
    assert(taskbar.upsert_app(1, "Files", "folder", false));
    assert(taskbar.upsert_app(2, "Settings", "settings", true));
    assert(taskbar.remove_app(1));
    assert(!taskbar.remove_app(999));
    assert(taskbar.set_app_state(2, true, true));
    assert(taskbar.handle_click(200, 560, 800, 600));
    assert(taskbar.activated_process_id() == 2);
    assert(taskbar_actions[BauraTaskbar::ActionActivateApp] == 1);
    taskbar.render(renderer);
    taskbar.toggle_start_menu();
    taskbar.render(renderer);
    taskbar.toggle_search();
    taskbar.render(renderer);
    taskbar.toggle_notifications();
    taskbar.render(renderer);

    BauraButton button(20, 20, 120, 36, "Continue");
    uint32_t button_calls = 0;
    button.set_action([](void *context) {
        ++*static_cast<uint32_t *>(context);
    }, &button_calls);
    button.draw(renderer);
    button.handle_event(EventType::CLICK, 30, 30);
    assert(button_calls == 1);
    BauraWindow decorated_window("BAURA", 80, 80, 320, 240);
    decorated_window.set_action_handler(window_action, &window_actions);
    decorated_window.set_work_area(0, 0, 800, 542);
    decorated_window.set_focused(true);
    assert(decorated_window.add_widget(&button));
    decorated_window.draw(renderer);
    decorated_window.handle_event(EventType::CLICK, 320, 90);
    assert(window_actions == 1);
    assert(last_window_action == BauraWindowAction::MINIMIZE);
    assert(decorated_window.minimized());
    decorated_window.restore();
    decorated_window.handle_event(EventType::CLICK, 350, 90);
    assert(last_window_action == BauraWindowAction::MAXIMIZE);
    assert(decorated_window.maximized());
    decorated_window.handle_event(EventType::CLICK, 750, 20);
    assert(!decorated_window.maximized());
    decorated_window.handle_event(EventType::CLICK, 380, 90);
    assert(last_window_action == BauraWindowAction::CLOSE);
    assert(decorated_window.closed());
    BauraWindow drag_window("Move me", 100, 100, 300, 200);
    drag_window.handle_event(EventType::CLICK, 130, 120);
    drag_window.handle_event(EventType::DRAG, 180, 160);
    drag_window.handle_event(EventType::RELEASE, 180, 160);
    assert(drag_window.bounds().x == 150 && drag_window.bounds().y == 140);

    BauraDesktop desktop;
    decorated_window.restore();
    desktop.taskbar().set_action_handler(taskbar_action, taskbar_actions);
    assert(desktop.add_window(&decorated_window));
    assert(desktop.add_window(&drag_window));
    desktop.render_frame(renderer, 1.5f);
    desktop.process_event(EventType::CLICK, 170, 150);
    desktop.process_event(EventType::DRAG, 220, 190);
    desktop.process_event(EventType::RELEASE, 220, 190);
    assert(drag_window.bounds().x == 200 && drag_window.bounds().y == 180);
    desktop.process_click(40, 560);
    assert(taskbar_actions[BauraTaskbar::ActionStartMenu] == 3);
    assert(desktop.remove_window(&drag_window));
    assert(desktop.remove_window(&decorated_window));

    const float graph[] = {0.0f, 0.5f, 1.0f, 0.25f};
    render_performance_graph(renderer, 10, 10, 120, 40, graph, 4,
                             0xFF55C99Au);
    BauraSystemBubble bubble;
    bubble.update_progress("Installing", 1.5f);
    bubble.render(renderer, 20, 20);
    bubble.set_visible(false);

    BauraLoginScreen login;
    login.set_user_name("Bryant");
    login.render(renderer);
    assert(login.submit() != 0);
    login.set_authentication_handler(
        [](void *, const char *user) {
            return user[0] == 'B' ? 0 : -1;
        }, nullptr);
    assert(login.submit() == 0);
    uint32_t login_calls = 0;
    login.set_credentials_handler(
        [](void *context, const char *user, const char *password) {
            ++*static_cast<uint32_t *>(context);
            return user[0] == 'B' && password[0] == 's' ? 0 : -1;
        }, &login_calls);
    assert(login.handle_click(400, 350, 800, 600));
    assert(login.handle_character('s'));
    assert(login.handle_click(400, 408, 800, 600));
    assert(login_calls == 1);

    BauraInstaller interactive_installer;
    interactive_installer.set_backend(installer_action, &installer_actions);
    assert(interactive_installer.handle_click(600, 455, 800, 600));
    assert(interactive_installer.stage() == BauraInstallStage::Partition);

    BauraInstaller installer;
    assert(!installer.run_stage());
    installer.set_backend(installer_action, &installer_actions);
    installer.render(renderer);
    assert(installer.run_stage());
    assert(installer.stage() == BauraInstallStage::Partition);
    assert(installer.run_stage());
    assert(installer.stage() == BauraInstallStage::Format);
    assert(installer.run_stage());
    assert(installer.run_stage());
    assert(installer.stage() == BauraInstallStage::Complete);
    assert(installer_actions == 5);
    installer.render(renderer);

    BauraInstaller failing_installer;
    uint32_t failures = 0;
    failing_installer.set_backend(
        [](void *context, BauraInstallStage) {
            ++*static_cast<uint32_t *>(context);
            return -7;
        }, &failures);
    assert(!failing_installer.run_stage());
    assert(failing_installer.stage() == BauraInstallStage::Failed);
    assert(failing_installer.last_error() == -7);
    assert(failing_installer.handle_click(600, 455, 800, 600));
    assert(failing_installer.stage() == BauraInstallStage::Failed);
    assert(failing_installer.last_error() == -7);
    assert(failures == 2);
    failing_installer.render(renderer);

    const char *preview_path = std::getenv("BAURA_GUI_PREVIEW_PATH");
    if (preview_path != nullptr) {
        BGEUIFramework preview_ui(renderer);
        if (preview_ui.create_window("BAURA OS preview",
                                     {120, 110, 480, 300}) == 0) {
            return 1;
        }
        preview_ui.render_desktop(0.0f);
        if (!write_bmp(preview_path, desktop_fb)) return 1;
    }
    return 0;
}
