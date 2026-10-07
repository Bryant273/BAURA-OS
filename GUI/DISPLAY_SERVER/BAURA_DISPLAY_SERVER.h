#ifndef BAURA_DISPLAY_SERVER_H
#define BAURA_DISPLAY_SERVER_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>

struct Surface {
    uint32_t id;
    uint32_t width;
    uint32_t height;
    std::unique_ptr<uint32_t[]> buffer;
};

enum DisplayStatus {
    DISPLAY_OK = 0,
    DISPLAY_ERR_ARGUMENT = -1,
    DISPLAY_ERR_UNAVAILABLE = -2,
    DISPLAY_ERR_NOT_FOUND = -3,
    DISPLAY_ERR_CAPACITY = -4
};

typedef int (*DisplayPresent)(void *context, uint32_t *pixels,
                              uint32_t width, uint32_t height,
                              uint32_t stride_pixels);

class DisplayServer {
public:
    DisplayServer();
    ~DisplayServer();

    int set_framebuffer(uint32_t *pixels, uint32_t width, uint32_t height,
                        uint32_t stride_pixels, size_t pixel_count,
                        DisplayPresent present, void *context);
    uint32_t create_surface(int width, int height);
    int update_surface(uint32_t id, const uint32_t *data,
                       size_t pixel_count);
    int destroy_surface(uint32_t id);
    int resize_surface(uint32_t id, int width, int height);
    int clear(uint32_t color);
    int draw_surface(uint32_t id, int x, int y);
    int flip_buffer();
    bool has_surface(uint32_t id) const;
    bool surface_size(uint32_t id, uint32_t &width, uint32_t &height) const;
    uint32_t width() const;
    uint32_t height() const;

private:
    uint32_t next_surface_id;
    uint32_t *screen_fb;
    uint32_t screen_width;
    uint32_t screen_height;
    uint32_t screen_stride;
    size_t surface_pixels;
    DisplayPresent present;
    void *present_context;
    std::map<uint32_t, Surface> surfaces;
};

#endif
