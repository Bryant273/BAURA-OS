#ifndef BAURA_GUI_FFI_H
#define BAURA_GUI_FFI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DisplayServer_t *DisplayServerHandle;
typedef struct WindowCompositor_t *CompositorHandle;

typedef int (*BauraGuiPresent)(void *context, uint32_t *pixels,
                               uint32_t width, uint32_t height,
                               uint32_t stride_pixels);

enum {
    BAURA_GUI_OK = 0,
    BAURA_GUI_ERR_ARGUMENT = -1,
    BAURA_GUI_ERR_UNAVAILABLE = -2,
    BAURA_GUI_ERR_NOT_FOUND = -3,
    BAURA_GUI_ERR_CAPACITY = -4
};

DisplayServerHandle gui_create_display_server(void);
void gui_destroy_display_server(DisplayServerHandle handle);
int gui_configure_framebuffer(DisplayServerHandle handle, uint32_t *pixels,
                              uint32_t width, uint32_t height,
                              uint32_t stride_pixels, size_t pixel_count,
                              BauraGuiPresent present, void *context);
uint32_t gui_create_surface(DisplayServerHandle handle, int width, int height);
int gui_update_surface(DisplayServerHandle handle, uint32_t id,
                       const uint32_t *pixels, size_t pixel_count);
int gui_destroy_surface(DisplayServerHandle handle, uint32_t id);
int gui_resize_surface(DisplayServerHandle handle, uint32_t id, int width,
                       int height);
int gui_get_framebuffer_size(DisplayServerHandle handle, uint32_t *width,
                             uint32_t *height);

CompositorHandle gui_create_compositor(DisplayServerHandle display);
void gui_destroy_compositor(CompositorHandle handle);
uint32_t gui_add_window(CompositorHandle handle, uint32_t surface_id,
                        int x, int y);
int gui_set_window_z_order(CompositorHandle handle, uint32_t window_id, int z);
int gui_move_window(CompositorHandle handle, uint32_t window_id, int x, int y);
int gui_set_window_visible(CompositorHandle handle, uint32_t window_id,
                           int visible);
int gui_set_window_surface(CompositorHandle handle, uint32_t window_id,
                           uint32_t surface_id);
int gui_remove_window(CompositorHandle handle, uint32_t window_id);
int gui_compose(CompositorHandle handle);

#ifdef __cplusplus
}
#endif

#endif
