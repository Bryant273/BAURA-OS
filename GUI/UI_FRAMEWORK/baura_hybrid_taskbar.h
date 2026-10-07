#ifndef BAURA_HYBRID_TASKBAR_H
#define BAURA_HYBRID_TASKBAR_H

#include "../BGE/renderer.h"

#include <cstdint>
#include <string>
#include <vector>

struct BauraTaskbarApp {
    uint64_t process_id;
    std::string title;
    std::string icon_id;
    bool active;
    bool focused;
};

struct BauraTaskbarStatus {
    bool network_connected;
    bool battery_present;
    uint8_t battery_percent;
    bool charging;
    std::string language;
    std::string date_time;
    int temperature_celsius;
    bool temperature_available;
};

struct BauraMediaWidget {
    std::string title;
    std::string artist;
    bool playing;
    bool visible;
};

class BauraTaskbar {
public:
    BauraTaskbar();
    bool upsert_app(uint64_t process_id, const std::string &title,
                    const std::string &icon_id, bool focused);
    bool remove_app(uint64_t process_id);
    bool set_app_state(uint64_t process_id, bool active, bool focused);
    uint64_t activated_process_id() const;
    bool set_status(const BauraTaskbarStatus &status);
    bool set_media(const BauraMediaWidget &media);
    void set_notification_count(uint32_t count);
    void toggle_start_menu();
    void toggle_search();
    void toggle_notifications();
    bool handle_search_character(char character);
    bool handle_search_backspace();
    const std::string &search_query() const;
    bool is_start_menu_open() const;
    bool is_search_open() const;
    bool are_notifications_open() const;
    bool handle_click(int x, int y, uint32_t screen_width,
                      uint32_t screen_height);
    void set_action_handler(void (*handler)(uint32_t action, void *context),
                            void *context);
    void render(BGERenderer &renderer);

    static constexpr uint32_t ActionStartMenu = 1;
    static constexpr uint32_t ActionSearch = 2;
    static constexpr uint32_t ActionNotifications = 3;
    static constexpr uint32_t ActionOptimize = 4;
    static constexpr uint32_t ActionMediaToggle = 5;
    static constexpr uint32_t ActionLaunchFiles = 6;
    static constexpr uint32_t ActionLaunchSettings = 7;
    static constexpr uint32_t ActionLaunchBrowser = 8;
    static constexpr uint32_t ActionLaunchInstaller = 9;
    static constexpr uint32_t ActionSearchQueryChanged = 10;
    static constexpr uint32_t ActionActivateApp = 11;

private:
    std::vector<BauraTaskbarApp> apps;
    BauraTaskbarStatus status;
    BauraMediaWidget media;
    std::string query;
    uint32_t notification_count;
    uint64_t activated_process;
    bool start_open;
    bool search_open;
    bool notifications_open;
    void (*action_handler)(uint32_t action, void *context);
    void *action_context;
    void draw_taskbar(BGERenderer &renderer);
    void draw_top_widgets(BGERenderer &renderer);
    void draw_start_panel(BGERenderer &renderer);
    void draw_search_panel(BGERenderer &renderer);
    void draw_notifications_panel(BGERenderer &renderer);
    void draw_icon(BGERenderer &renderer, const char *icon, int x, int y,
                   int size);
    void emit_action(uint32_t action);
};

#endif
