#ifndef BAURA_LOGIN_SCREEN_H
#define BAURA_LOGIN_SCREEN_H

#include "../BGE/renderer.h"

#include <string>

class BauraLoginScreen {
public:
    BauraLoginScreen();
    void set_user_name(const std::string &name);
    void set_message(const std::string &message);
    void set_authentication_handler(
        int (*handler)(void *context, const char *user_name),
        void *context);
    void set_credentials_handler(
        int (*handler)(void *context, const char *user_name,
                       const char *password),
        void *context);
    bool handle_click(int x, int y, uint32_t screen_width,
                      uint32_t screen_height);
    bool handle_character(char character);
    bool handle_backspace();
    void clear_password();
    void render(BGERenderer &renderer);
    int submit();

private:
    std::string user_name;
    std::string message;
    int (*authenticate)(void *context, const char *user_name);
    int (*authenticate_credentials)(void *context, const char *user_name,
                                    const char *password);
    void *auth_context;
    std::string password;
    bool password_focused;
};

#endif
