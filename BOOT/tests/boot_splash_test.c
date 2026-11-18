#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../FIRMWARE/FRAMEBUFFER/baura_boot_splash.h"

int main(void) {
    /* 1. Test validation */
    assert(baura_boot_framebuffer_validate(NULL) == BAURA_BOOT_SPLASH_ERR_ARGUMENT);

    BauraBootFramebuffer fb = {0};
    assert(baura_boot_framebuffer_validate(&fb) == BAURA_BOOT_SPLASH_ERR_ARGUMENT);

    uint32_t buffer[640 * 480];
    fb.pixels = buffer;
    fb.width = 640;
    fb.height = 480;
    fb.pitch = 640 * 4;
    fb.bpp = 24; /* invalid */
    assert(baura_boot_framebuffer_validate(&fb) == BAURA_BOOT_SPLASH_ERR_UNSUPPORTED);

    fb.bpp = 32;
    fb.width = 200; /* too small */
    assert(baura_boot_framebuffer_validate(&fb) == BAURA_BOOT_SPLASH_ERR_ARGUMENT);

    fb.width = 640;
    fb.pitch = 640 * 2; /* pitch too small */
    assert(baura_boot_framebuffer_validate(&fb) == BAURA_BOOT_SPLASH_ERR_ARGUMENT);

    fb.pitch = 640 * 4;
    assert(baura_boot_framebuffer_validate(&fb) == BAURA_BOOT_SPLASH_OK);

    /* 2. Test draw splash in RGBX */
    memset(buffer, 0, sizeof(buffer));
    fb.format = BAURA_BOOT_FB_FORMAT_RGBX;
    assert(baura_boot_draw_splash(&fb, "BAURA OS x86_64 BOOT", 50) == BAURA_BOOT_SPLASH_OK);

    /* Verify pixels were rendered */
    int non_zero = 0;
    for (size_t i = 0; i < 640 * 480; ++i) {
        if (buffer[i] != 0) ++non_zero;
    }
    assert(non_zero > 1000);

    /* 3. Test draw splash in BGRX */
    memset(buffer, 0, sizeof(buffer));
    fb.format = BAURA_BOOT_FB_FORMAT_BGRX;
    assert(baura_boot_draw_splash(&fb, "STARTING SYSTEM SERVICES", 100) == BAURA_BOOT_SPLASH_OK);

    non_zero = 0;
    for (size_t i = 0; i < 640 * 480; ++i) {
        if (buffer[i] != 0) ++non_zero;
    }
    assert(non_zero > 1000);

    /* 4. Test individual primitives bounds */
    baura_boot_draw_pixel(&fb, -1, 0, 0xFFFFFFFF);
    baura_boot_draw_pixel(&fb, 640, 480, 0xFFFFFFFF);
    baura_boot_draw_string(&fb, -10, -10, "OFFSCREEN", 0xFFFFFFFF, 1);
    baura_boot_draw_mark(&fb, -20, -20, 32);

    return 0;
}
