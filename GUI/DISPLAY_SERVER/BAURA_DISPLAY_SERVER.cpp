
#include "BAURA_DISPLAY_SERVER.h"
#include <iostream>

DisplayServer::DisplayServer() {
    // Simulation: Map the hardware framebuffer to a memory region
    screen_fb = new uint32_t[1920 * 1080];
    std::cout << "[DisplayServer] Hardware Framebuffer Initialized (1920x1080)" << std::endl;
}

DisplayServer::~DisplayServer() {
    delete[] screen_fb;
}

uint32_t DisplayServer::create_surface(int w, int h) {
    uint32_t id = next_surface_id++;
    Surface s = { id, w, h, new uint32_t[w * h] };
    surfaces[id] = std::make_unique<Surface>(s);
    return id;
}

void DisplayServer::update_surface(uint32_t id, uint32_t* data) {
    if (surfaces.count(id)) {
        // Copy data to the internal surface buffer
        std::copy(data, data + (surfaces[id]->width * surfaces[id]->height), surfaces[id]->buffer);
    }
}

void DisplayServer::destroy_surface(uint32_t id) {
    surfaces.erase(id);
}

void DisplayServer::flip_buffer() {
    // In a real OS, this would trigger a GPU VSync and flip the buffers
    std::cout << "[DisplayServer] Buffer Flip performed." << std::endl;
}
