#include "baura_hybrid_taskbar.h"

#include "baura_theme.h"
#include "papirus_icon_catalog.h"

#include <algorithm>
#include <cstdio>
#include <limits>
#include <new>
#include <utility>

static void draw_text_prefix(BGERenderer &renderer, int x, int y,
                             const std::string &text, size_t maximum,
                             uint32_t color) {
    char buffer[64];
    const size_t length = std::min(maximum, sizeof(buffer) - 1);
    const size_t copied = std::min(length, text.size());
    std::copy_n(text.data(), copied, buffer);
    buffer[copied] = '\0';
    renderer.draw_text(x, y, buffer, color);
}

BauraTaskbar::BauraTaskbar()
    : status{false, false, 0, false, "EN", "--:--", 0, false},
      media{"", "", false, false},
      query(),
      notification_count(0),
      activated_process(0),
      start_open(false),
      search_open(false),
      notifications_open(false),
      action_handler(nullptr),
      action_context(nullptr) {}

bool BauraTaskbar::upsert_app(uint64_t process_id, const std::string &title,
                              const std::string &icon_id, bool focused) {
    if (process_id == 0 || title.empty()) return false;
    for (BauraTaskbarApp &app : apps) {
        if (app.process_id == process_id) {
            std::string next_title;
            std::string next_icon;
            try {
                next_title = title;
                next_icon = icon_id;
            } catch (const std::bad_alloc &) {
                return false;
            }
            app.title.swap(next_title);
            app.icon_id.swap(next_icon);
            app.focused = focused;
            app.active = true;
            return true;
        }
    }
    try {
        apps.push_back({process_id, title, icon_id, true, focused});
    } catch (const std::bad_alloc &) {
        return false;
    }
    return true;
}

bool BauraTaskbar::remove_app(uint64_t process_id) {
    for (auto app = apps.begin(); app != apps.end(); ++app) {
        if (app->process_id == process_id) {
            apps.erase(app);
            if (activated_process == process_id) activated_process = 0;
            return true;
        }
    }
    return false;
}

bool BauraTaskbar::set_app_state(uint64_t process_id, bool active,
                                 bool focused) {
    for (BauraTaskbarApp &app : apps) {
        if (app.process_id == process_id) {
            app.active = active;
            app.focused = active && focused;
            if (!active && activated_process == process_id) {
                activated_process = 0;
            }
            return true;
        }
    }
    return false;
}

uint64_t BauraTaskbar::activated_process_id() const {
    return activated_process;
}

bool BauraTaskbar::set_status(const BauraTaskbarStatus &value) {
    BauraTaskbarStatus updated;
    try {
        updated = value;
    } catch (const std::bad_alloc &) {
        return false;
    }
    status = std::move(updated);
    if (status.battery_percent > 100) status.battery_percent = 100;
    return true;
}

bool BauraTaskbar::set_media(const BauraMediaWidget &value) {
    BauraMediaWidget updated;
    try {
        updated = value;
    } catch (const std::bad_alloc &) {
        return false;
    }
    media = std::move(updated);
    return true;
}
void BauraTaskbar::set_notification_count(uint32_t count) {
    notification_count = count;
}

void BauraTaskbar::toggle_start_menu() {
    start_open = !start_open;
    if (start_open) {
        search_open = false;
        notifications_open = false;
    }
    emit_action(ActionStartMenu);
}

void BauraTaskbar::toggle_search() {
    search_open = !search_open;
    if (!search_open) query.clear();
    if (search_open) {
        start_open = false;
        notifications_open = false;
    }
    emit_action(ActionSearch);
}

void BauraTaskbar::toggle_notifications() {
    notifications_open = !notifications_open;
    if (notifications_open) {
        start_open = false;
        search_open = false;
    }
    emit_action(ActionNotifications);
}

bool BauraTaskbar::handle_search_character(char character) {
    if (!search_open || character < 0x20 || character > 0x7E ||
        query.size() >= 128) return false;
    query.push_back(character);
    emit_action(ActionSearchQueryChanged);
    return true;
}

bool BauraTaskbar::handle_search_backspace() {
    if (!search_open || query.empty()) return false;
    query.pop_back();
    emit_action(ActionSearchQueryChanged);
    return true;
}

const std::string &BauraTaskbar::search_query() const { return query; }
bool BauraTaskbar::is_start_menu_open() const { return start_open; }
bool BauraTaskbar::is_search_open() const { return search_open; }
bool BauraTaskbar::are_notifications_open() const {
    return notifications_open;
}

bool BauraTaskbar::handle_click(int x, int y, uint32_t screen_width,
                                uint32_t screen_height) {
    if (screen_width < 120 || screen_height < 58 ||
        screen_width > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
        screen_height > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
        return false;
    }
    if (start_open && y >= static_cast<int>(screen_height) - 350 &&
        y < static_cast<int>(screen_height) - 58 && x >= 38 && x < 324) {
        const int panel_y = static_cast<int>(screen_height) - 350;
        const int column = x < 182 ? 0 : 1;
        const int row = y >= panel_y + 196 ? 1 : 0;
        if (y >= panel_y + 124 && y < panel_y + 268) {
            const uint32_t actions[4] = {
                ActionLaunchFiles, ActionLaunchSettings,
                ActionLaunchBrowser, ActionLaunchInstaller
            };
            start_open = false;
            emit_action(actions[row * 2 + column]);
            return true;
        }
    }
    if (search_open && y >= static_cast<int>(screen_height) - 172 &&
        y < static_cast<int>(screen_height) - 134 && x >= 34 && x < 382) {
        return true;
    }
    if (notifications_open && screen_height >= 310 &&
        y >= 214 && y < 256) {
        const int panel_width = std::min(320, static_cast<int>(screen_width) - 24);
        const int panel_x = static_cast<int>(screen_width) - panel_width - 12;
        if (x >= panel_x + 18 && x < panel_x + panel_width - 18) {
            emit_action(ActionOptimize);
            return true;
        }
    }
    if (y >= static_cast<int>(screen_height) - 58 && y < screen_height) {
        if (x >= 22 && x < 58) {
            toggle_start_menu();
            return true;
        }
        if (x >= 64 && x < 174) {
            toggle_search();
            return true;
        }
        int app_x = 184;
        for (const BauraTaskbarApp &app : apps) {
            if (app_x + 42 >= static_cast<int>(screen_width) - 230) break;
            if (y >= static_cast<int>(screen_height) - 47 &&
                y < static_cast<int>(screen_height) - 11 &&
                x >= app_x && x < app_x + 38) {
                activated_process = app.process_id;
                emit_action(ActionActivateApp);
                return true;
            }
            app_x += 44;
        }
    }
    if (y >= 15 && y < 49) {
        int cursor = static_cast<int>(screen_width) - 18;
        if (media.visible) {
            const int width = std::min(220, static_cast<int>(screen_width) / 2);
            if (width >= 100 && x >= cursor - width && x < cursor) {
                media.playing = !media.playing;
                emit_action(ActionMediaToggle);
                return true;
            }
            if (width >= 100) cursor -= width;
        }
        if (x >= cursor - 38 && x < cursor - 4) {
            toggle_notifications();
            return true;
        }
        if (x >= cursor - 151 && x < cursor - 117) {
            emit_action(ActionOptimize);
            return true;
        }
    }
    return false;
}

void BauraTaskbar::set_action_handler(
    void (*handler)(uint32_t action, void *context), void *context) {
    action_handler = handler;
    action_context = context;
}

void BauraTaskbar::emit_action(uint32_t action) {
    if (action_handler != nullptr) action_handler(action, action_context);
}

void BauraTaskbar::draw_icon(BGERenderer &renderer, const char *icon, int x,
                             int y, int size) {
    const BauraIconEntry *entry = baura_icon_find(icon);
    renderer.draw_icon(x, y, size,
                       entry == nullptr ? BauraTheme::TextPrimary : entry->tint,
                       entry == nullptr ? icon : entry->papirus_name);
}

void BauraTaskbar::draw_taskbar(BGERenderer &renderer) {
    const BGEFramebuffer &fb = renderer.framebuffer();
    const int width = static_cast<int>(fb.width);
    if (width < 120 || fb.height < 58) return;
    const int bar_height = 58;
    const int y = static_cast<int>(fb.height) - bar_height;
    renderer.draw_rounded_rect(12, y + 5, width - 24, bar_height - 10, 18,
                               BauraTheme::GlassDark, 235);
    renderer.draw_rounded_rect(12, y + 5, width - 24, bar_height - 10, 18,
                               BauraTheme::Border, 80);

    renderer.draw_rounded_rect(22, y + 12, 36, 34, 11,
                               start_open ? BauraTheme::AccentBlue
                                          : BauraTheme::SurfaceRaised);
    renderer.draw_text(31, y + 24, "B", BauraTheme::TextPrimary, 1);
    renderer.draw_rounded_rect(64, y + 12, 110, 34, 12,
                               BauraTheme::SurfaceRaised, 210);
    draw_icon(renderer, "search", 69, y + 16, 24);
    renderer.draw_text(96, y + 25, "Search", BauraTheme::TextSecondary);

    int app_x = 184;
    const int right_reserve = 230;
    for (const BauraTaskbarApp &app : apps) {
        if (app_x + 42 >= width - right_reserve) break;
        renderer.draw_rounded_rect(app_x, y + 11, 38, 36, 10,
                                   app.focused ? BauraTheme::AccentBlue
                                               : BauraTheme::SurfaceRaised,
                                   app.focused ? 220 : 175);
        draw_icon(renderer, app.icon_id.c_str(), app_x + 7, y + 16, 24);
        if (app.active) {
            renderer.draw_rounded_rect(app_x + 16, y + 45, 6, 2, 1,
                                       app.focused ? BauraTheme::AccentCyan
                                                   : BauraTheme::TextSecondary);
        }
        app_x += 44;
    }

    int right = width - 30;
    if (!status.date_time.empty()) {
        const size_t time_length = std::min<size_t>(status.date_time.size(), 16);
        const int time_width = static_cast<int>(time_length * 6);
        right -= time_width;
        draw_text_prefix(renderer, right, y + 25, status.date_time, 16,
                         BauraTheme::TextPrimary);
    }
    if (!status.language.empty()) {
        right -= 25;
        draw_text_prefix(renderer, right, y + 25, status.language, 4,
                         BauraTheme::TextSecondary);
    }
    if (status.battery_present) {
        right -= 42;
        draw_icon(renderer, "battery", right, y + 16, 24);
        char percent[5] = {
            static_cast<char>('0' + status.battery_percent / 100),
            static_cast<char>('0' + (status.battery_percent / 10) % 10),
            static_cast<char>('0' + status.battery_percent % 10), '%', '\0'
        };
        const char *shown = status.battery_percent == 100 ? percent : percent + 1;
        renderer.draw_text(right + 24, y + 25, shown,
                           status.charging ? BauraTheme::Success
                                           : BauraTheme::TextSecondary);
    }
    right -= 28;
    draw_icon(renderer, status.network_connected ? "network" : "network-off",
              right, y + 16, 24);
}

void BauraTaskbar::draw_top_widgets(BGERenderer &renderer) {
    const BGEFramebuffer &fb = renderer.framebuffer();
    int x = static_cast<int>(fb.width) - 18;
    const int y = 12;
    if (media.visible) {
        const int width = std::min(220, static_cast<int>(fb.width) / 2);
        if (width >= 100) {
            x -= width;
            renderer.draw_rounded_rect(x, y, width, 42, 14,
                                       BauraTheme::GlassDark, 225);
            draw_icon(renderer, media.playing ? "media-pause" : "media-play",
                      x + 8, y + 8, 25);
            draw_text_prefix(renderer, x + 38, y + 10, media.title, 30,
                             BauraTheme::TextPrimary);
            draw_text_prefix(renderer, x + 38, y + 23, media.artist, 30,
                             BauraTheme::TextSecondary);
        }
    }
    x -= 38;
    renderer.draw_rounded_rect(x, y + 3, 34, 34, 12,
                               notifications_open ? BauraTheme::AccentBlue
                                                  : BauraTheme::GlassDark,
                               230);
    draw_icon(renderer, "notifications", x + 5, y + 8, 24);
    if (notification_count != 0) {
        renderer.draw_rounded_rect(x + 24, y, 14, 14, 7,
                                   BauraTheme::Danger);
        char count[2] = {
            static_cast<char>('0' + std::min(notification_count, 9u)), '\0'
        };
        renderer.draw_text(x + 27, y + 4, count, BauraTheme::White);
    }
    x -= 76;
    renderer.draw_rounded_rect(x, y + 3, 68, 34, 12,
                               BauraTheme::GlassDark, 220);
    if (status.temperature_available) {
        char value[16] = {0};
        const int temperature =
            std::max(-99, std::min(999, status.temperature_celsius));
        std::snprintf(value, sizeof(value), "%dC", temperature);
        renderer.draw_text(x + 6, y + 16, value, BauraTheme::TextPrimary);
    } else {
        renderer.draw_text(x + 6, y + 16, "--C", BauraTheme::TextSecondary);
    }
    renderer.draw_rounded_rect(x - 37, y + 3, 34, 34, 12,
                               BauraTheme::GlassDark, 220);
    renderer.draw_text(x - 27, y + 15, "+", BauraTheme::AccentCyan, 2);
}

void BauraTaskbar::draw_start_panel(BGERenderer &renderer) {
    const int y = static_cast<int>(renderer.framebuffer().height) - 350;
    renderer.draw_rounded_rect(18, y, 330, 330, 18, BauraTheme::GlassDark, 245);
    renderer.draw_text(40, y + 24, "BAURA", BauraTheme::TextPrimary, 2);
    renderer.draw_rounded_rect(38, y + 62, 286, 38, 12,
                               BauraTheme::SurfaceRaised);
    draw_icon(renderer, "search", 44, y + 68, 24);
    renderer.draw_text(74, y + 79, "Search apps", BauraTheme::TextSecondary);
    static const char *apps[] = {"Files", "Settings", "Browser", "Install"};
    static const char *icons[] = {"folder", "settings", "network", "install"};
    for (unsigned index = 0; index < 4; ++index) {
        const int column = static_cast<int>(index % 2);
        const int row = static_cast<int>(index / 2);
        const int item_x = 42 + column * 140;
        const int item_y = y + 124 + row * 72;
        draw_icon(renderer, icons[index], item_x, item_y, 32);
        renderer.draw_text(item_x + 38, item_y + 12, apps[index],
                           BauraTheme::TextPrimary);
    }
}

void BauraTaskbar::draw_search_panel(BGERenderer &renderer) {
    const int y = static_cast<int>(renderer.framebuffer().height) - 190;
    renderer.draw_rounded_rect(18, y, 380, 165, 18, BauraTheme::GlassDark, 245);
    renderer.draw_rounded_rect(34, y + 18, 348, 38, 12,
                               BauraTheme::SurfaceRaised);
    draw_icon(renderer, "search", 42, y + 25, 24);
    if (query.empty()) {
        renderer.draw_text(74, y + 35, "Search apps, settings and files",
                           BauraTheme::TextSecondary);
    } else {
        draw_text_prefix(renderer, 74, y + 35, query, 48,
                         BauraTheme::TextPrimary);
    }
}

void BauraTaskbar::draw_notifications_panel(BGERenderer &renderer) {
    const BGEFramebuffer &fb = renderer.framebuffer();
    const int width = std::min(320, static_cast<int>(fb.width) - 24);
    if (width < 100 || fb.height < 310) return;
    const int x = static_cast<int>(fb.width) - width - 12;
    renderer.draw_rounded_rect(x, 58, width, 230, 18, BauraTheme::GlassDark, 245);
    renderer.draw_text(x + 20, 80, "Notifications", BauraTheme::TextPrimary, 2);
    if (notification_count == 0) {
        renderer.draw_text(x + 20, 130, "You are all caught up",
                           BauraTheme::TextSecondary);
    } else {
        renderer.draw_text(x + 20, 130, "New system activity",
                           BauraTheme::TextSecondary);
        renderer.draw_text(x + 20, 150, "Open notification center",
                           BauraTheme::AccentCyan);
    }
    renderer.draw_rounded_rect(x + 18, 214, width - 36, 42, 12,
                               BauraTheme::SurfaceRaised);
    renderer.draw_text(x + 32, 231, "Optimize", BauraTheme::TextPrimary);
}

void BauraTaskbar::render(BGERenderer &renderer) {
    if (!renderer.valid()) return;
    draw_taskbar(renderer);
    draw_top_widgets(renderer);
    if (start_open) draw_start_panel(renderer);
    if (search_open) draw_search_panel(renderer);
    if (notifications_open) draw_notifications_panel(renderer);
}
