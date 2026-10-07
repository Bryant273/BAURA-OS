#include "BAURA_GUI_FFI.h"

#include "COMPOSITOR/BAURA_COMPOSITOR.h"
#include "DISPLAY_SERVER/BAURA_DISPLAY_SERVER.h"

#include <memory>
#include <new>
#include <utility>

struct DisplayServer_t {
    std::shared_ptr<DisplayServer> instance;
};

struct WindowCompositor_t {
    std::shared_ptr<DisplayServer> display;
    std::unique_ptr<WindowCompositor> instance;
};

extern "C" DisplayServerHandle gui_create_display_server(void) {
    try {
        std::shared_ptr<DisplayServer> display =
            std::make_shared<DisplayServer>();
        DisplayServer_t *handle = new (std::nothrow) DisplayServer_t{
            std::move(display)};
        return handle;
    } catch (const std::bad_alloc &) {
        return nullptr;
    }
}

extern "C" void gui_destroy_display_server(DisplayServerHandle handle) {
    delete handle;
}

extern "C" int gui_configure_framebuffer(
    DisplayServerHandle handle, uint32_t *pixels, uint32_t width,
    uint32_t height, uint32_t stride_pixels, size_t pixel_count,
    BauraGuiPresent present, void *context) {
    if (handle == nullptr || handle->instance == nullptr) {
        return BAURA_GUI_ERR_ARGUMENT;
    }
    return handle->instance->set_framebuffer(
        pixels, width, height, stride_pixels, pixel_count, present, context);
}

extern "C" uint32_t gui_create_surface(DisplayServerHandle handle,
                                       int width, int height) {
    if (handle == nullptr || handle->instance == nullptr) return 0;
    return handle->instance->create_surface(width, height);
}

extern "C" int gui_update_surface(DisplayServerHandle handle, uint32_t id,
                                  const uint32_t *pixels,
                                  size_t pixel_count) {
    if (handle == nullptr || handle->instance == nullptr) {
        return BAURA_GUI_ERR_ARGUMENT;
    }
    return handle->instance->update_surface(id, pixels, pixel_count);
}

extern "C" int gui_destroy_surface(DisplayServerHandle handle, uint32_t id) {
    if (handle == nullptr || handle->instance == nullptr) {
        return BAURA_GUI_ERR_ARGUMENT;
    }
    return handle->instance->destroy_surface(id);
}

extern "C" int gui_resize_surface(DisplayServerHandle handle, uint32_t id,
                                  int width, int height) {
    if (handle == nullptr || handle->instance == nullptr) {
        return BAURA_GUI_ERR_ARGUMENT;
    }
    return handle->instance->resize_surface(id, width, height);
}

extern "C" int gui_get_framebuffer_size(DisplayServerHandle handle,
                                        uint32_t *width, uint32_t *height) {
    if (handle == nullptr || handle->instance == nullptr ||
        width == nullptr || height == nullptr) {
        return BAURA_GUI_ERR_ARGUMENT;
    }
    *width = handle->instance->width();
    *height = handle->instance->height();
    if (*width == 0 || *height == 0) return BAURA_GUI_ERR_UNAVAILABLE;
    return BAURA_GUI_OK;
}

extern "C" CompositorHandle gui_create_compositor(
    DisplayServerHandle display) {
    if (display == nullptr || display->instance == nullptr) return nullptr;
    try {
        std::unique_ptr<WindowCompositor> compositor(
            new (std::nothrow) WindowCompositor(display->instance.get()));
        if (!compositor) return nullptr;
        return new (std::nothrow) WindowCompositor_t{
            display->instance, std::move(compositor)};
    } catch (const std::bad_alloc &) {
        return nullptr;
    }
}

extern "C" void gui_destroy_compositor(CompositorHandle handle) {
    delete handle;
}

extern "C" uint32_t gui_add_window(CompositorHandle handle,
                                   uint32_t surface_id, int x, int y) {
    if (handle == nullptr || handle->instance == nullptr) return 0;
    return handle->instance->add_window(surface_id, x, y);
}

extern "C" int gui_set_window_z_order(CompositorHandle handle,
                                      uint32_t window_id, int z) {
    if (handle == nullptr || handle->instance == nullptr) {
        return BAURA_GUI_ERR_ARGUMENT;
    }
    return handle->instance->set_z_order(window_id, z);
}

extern "C" int gui_move_window(CompositorHandle handle, uint32_t window_id,
                               int x, int y) {
    if (handle == nullptr || handle->instance == nullptr) {
        return BAURA_GUI_ERR_ARGUMENT;
    }
    return handle->instance->move_window(window_id, x, y);
}

extern "C" int gui_set_window_visible(CompositorHandle handle,
                                      uint32_t window_id, int visible) {
    if (handle == nullptr || handle->instance == nullptr ||
        (visible != 0 && visible != 1)) {
        return BAURA_GUI_ERR_ARGUMENT;
    }
    return handle->instance->set_visible(window_id, visible != 0);
}

extern "C" int gui_set_window_surface(CompositorHandle handle,
                                      uint32_t window_id,
                                      uint32_t surface_id) {
    if (handle == nullptr || handle->instance == nullptr) {
        return BAURA_GUI_ERR_ARGUMENT;
    }
    return handle->instance->set_surface(window_id, surface_id);
}

extern "C" int gui_remove_window(CompositorHandle handle,
                                 uint32_t window_id) {
    if (handle == nullptr || handle->instance == nullptr) {
        return BAURA_GUI_ERR_ARGUMENT;
    }
    return handle->instance->remove_window(window_id);
}

extern "C" int gui_compose(CompositorHandle handle) {
    if (handle == nullptr || handle->instance == nullptr) {
        return BAURA_GUI_ERR_ARGUMENT;
    }
    return handle->instance->compose();
}
