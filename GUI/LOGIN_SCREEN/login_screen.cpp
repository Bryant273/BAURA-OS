#include "login_screen.h"

#include "../UI_FRAMEWORK/baura_theme.h"

#include <algorithm>
#include <cstdint>
#include <new>

static void draw_login_text(BGERenderer &renderer, int x, int y,
                            const std::string &text, size_t maximum,
                            uint32_t color, unsigned scale = 1) {
    char buffer[96];
    const size_t length = std::min(maximum, sizeof(buffer) - 1);
    const size_t copied = std::min(length, text.size());
    std::copy_n(text.data(), copied, buffer);
    buffer[copied] = '\0';
    renderer.draw_text(x, y, buffer, color, scale);
}

BauraLoginScreen::BauraLoginScreen()
    : user_name("User"),
      message(),
      authenticate(nullptr),
      authenticate_credentials(nullptr),
      auth_context(nullptr),
      password(),
      password_focused(false) {}

void BauraLoginScreen::set_user_name(const std::string &name) {
    user_name = name;
}

void BauraLoginScreen::set_message(const std::string &text) { message = text; }

void BauraLoginScreen::set_authentication_handler(
    int (*handler)(void *, const char *), void *context) {
    authenticate = handler;
    authenticate_credentials = nullptr;
    auth_context = context;
}

void BauraLoginScreen::set_credentials_handler(
    int (*handler)(void *, const char *, const char *), void *context) {
    authenticate_credentials = handler;
    authenticate = nullptr;
    auth_context = context;
}

bool BauraLoginScreen::handle_click(int x, int y, uint32_t screen_width,
                                    uint32_t screen_height) {
    if (screen_width == 0 || screen_height == 0 ||
        screen_width > static_cast<uint32_t>(INT32_MAX) ||
        screen_height > static_cast<uint32_t>(INT32_MAX)) return false;
    const int panel_x = (static_cast<int>(screen_width) - 360) / 2;
    const int panel_y = (static_cast<int>(screen_height) - 300) / 2;
    if (x >= panel_x + 30 && x < panel_x + 330 &&
        y >= panel_y + 160 && y < panel_y + 202) {
        password_focused = true;
        return true;
    }
    if (x >= panel_x + 30 && x < panel_x + 330 &&
        y >= panel_y + 218 && y < panel_y + 260) {
        submit();
        return true;
    }
    password_focused = false;
    return false;
}

bool BauraLoginScreen::handle_character(char character) {
    if (!password_focused || character < 0x20 || character > 0x7E ||
        password.size() >= 128) return false;
    try {
        password.push_back(character);
    } catch (const std::bad_alloc &) {
        return false;
    }
    message.clear();
    return true;
}

bool BauraLoginScreen::handle_backspace() {
    if (!password_focused || password.empty()) return false;
    password.back() = '\0';
    password.pop_back();
    return true;
}

void BauraLoginScreen::clear_password() {
    volatile char *secret = password.data();
    for (size_t index = 0; index < password.size(); ++index) {
        secret[index] = '\0';
    }
    password.clear();
}

void BauraLoginScreen::render(BGERenderer &renderer) {
    if (!renderer.valid()) return;
    const BGEFramebuffer &fb = renderer.framebuffer();
    renderer.draw_gradient_rect(0, 0, static_cast<int>(fb.width),
                                static_cast<int>(fb.height), 0xFF14263Fu,
                                0xFF202039u);
    const int panel_width = 360;
    const int panel_height = 300;
    const int x = (static_cast<int>(fb.width) - panel_width) / 2;
    const int y = (static_cast<int>(fb.height) - panel_height) / 2;
    renderer.draw_rounded_rect(x, y, panel_width, panel_height, 22,
                               BauraTheme::GlassDark, 238);
    renderer.draw_rounded_rect(x + panel_width / 2 - 30, y + 32, 60, 60, 30,
                               BauraTheme::AccentBlue);
    renderer.draw_icon(x + panel_width / 2 - 16, y + 46, 32,
                       BauraTheme::TextPrimary, "user");
    draw_login_text(renderer, x + 36, y + 116, user_name, 32,
                    BauraTheme::TextPrimary, 2);
    renderer.draw_rounded_rect(x + 30, y + 160, panel_width - 60, 42, 12,
                               BauraTheme::SurfaceRaised);
    if (password_focused) {
        renderer.draw_rounded_rect(x + 30, y + 160, panel_width - 60, 42, 12,
                                   BauraTheme::AccentCyan, 90);
    }
    if (password.empty()) {
        renderer.draw_text(x + 46, y + 177, "Enter password",
                           BauraTheme::TextSecondary);
    } else {
        const size_t shown = std::min(password.size(), size_t{32});
        char masked[33];
        std::fill_n(masked, shown, '*');
        masked[shown] = '\0';
        renderer.draw_text(x + 46, y + 177, masked, BauraTheme::TextPrimary);
    }
    renderer.draw_rounded_rect(x + 30, y + 218, panel_width - 60, 42, 12,
                               BauraTheme::AccentBlue);
    renderer.draw_text(x + 145, y + 235, "Sign in",
                       BauraTheme::TextPrimary);
    if (!message.empty()) {
        draw_login_text(renderer, x + 30, y + 280, message, 48,
                        BauraTheme::Warning);
    }
}

int BauraLoginScreen::submit() {
    if (authenticate_credentials == nullptr && authenticate == nullptr) {
        message = "Authentication service unavailable";
        clear_password();
        return -1;
    }
    const int status = authenticate_credentials != nullptr
                           ? authenticate_credentials(
                                 auth_context, user_name.c_str(),
                                 password.c_str())
                           : authenticate(auth_context, user_name.c_str());
    clear_password();
    if (status != 0) message = "Sign-in failed";
    else message.clear();
    return status;
}
