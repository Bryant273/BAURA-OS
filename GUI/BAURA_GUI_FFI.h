
#ifndef BAURA_GUI_FFI_H
#define BAURA_GUI_FFI_H

#include <stdint.h>

// Handle to the Display Server
typedef struct DisplayServer_t* DisplayServerHandle;
typedef struct WindowCompositor_t* CompositorHandle;

// Display Server API
extern "C" {
    DisplayServerHandle gui_create_display_server();
    uint32_t gui_create_surface(DisplayServerHandle handle, int w, int h);
    void gui_update_surface(DisplayServerHandle handle, uint32_t id, uint32_t* data);
}

// Compositor API
extern "C" {
    CompositorHandle gui_create_compositor(DisplayServerHandle ds_handle);
    uint32_t gui_add_window(CompositorHandle handle, uint32_t surface_id, int x, int y);
    void gui_compose(CompositorHandle handle);
}

#endif
