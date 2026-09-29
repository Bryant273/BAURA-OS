
#ifndef BAURA_DISPLAY_SERVER_H
#define BAURA_DISPLAY_SERVER_H

#include <vector>
#include <string>
#include <memory>
#include <map>

struct Surface {
    uint32_t id;
    int width, height;
    uint32_t* buffer;
};

class DisplayServer {
public:
    DisplayServer();
    ~DisplayServer();

    uint32_t create_surface(int w, int h);
    void update_surface(uint32_t id, uint32_t* data);
    void destroy_surface(uint32_t id);

    // Hardware abstraction for the actual framebuffer
    void flip_buffer();

private:
    uint32_t next_surface_id = 1;
    std::map<uint32_t, std::unique_ptr<Surface>> surfaces;
    uint32_t* screen_fb = nullptr;
};

#endif
