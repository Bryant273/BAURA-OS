#include "BAURA_UI_FRAMEWORK.h"

#include "../UI_FRAMEWORK/papirus_icon_catalog.h"

#include <algorithm>
#include <climits>
#include <new>

static bool valid_widget_bounds(int x, int y, int width, int height) {
    return width > 0 && height > 0 &&
           static_cast<int64_t>(x) + width <= INT_MAX &&
           static_cast<int64_t>(y) + height <= INT_MAX;
}

static bool valid_window_bounds(int x, int y, int width, int height) {
    return width >= 120 && height >= 72 &&
           valid_widget_bounds(x, y, width, height);
}

BauraWidget::BauraWidget(int origin_x, int origin_y, int widget_width,
                         int widget_height)
    : x(origin_x),
      y(origin_y),
      width(widget_width),
      height(widget_height),
      visible(true),
      enabled(true) {}

bool BauraWidget::contains(int px, int py) const {
    return width > 0 && height > 0 && px >= x && py >= y &&
           static_cast<int64_t>(px) < static_cast<int64_t>(x) + width &&
           static_cast<int64_t>(py) < static_cast<int64_t>(y) + height;
}

void BauraWidget::handle_event(EventType, int, int) {}

BauraButton::BauraButton(int origin_x, int origin_y, int button_width,
                         int button_height, const std::string &text)
    : BauraWidget(origin_x, origin_y, button_width, button_height),
      label(text),
      hovered(false),
      is_pressed(false),
      action(nullptr),
      action_context(nullptr) {}

void BauraButton::draw(BGERenderer &renderer) {
    if (!visible || width < 24 || height < 20 ||
        !valid_widget_bounds(x, y, width, height)) return;
    const uint32_t background = !enabled ? BauraTheme::Surface
                                  : is_pressed ? BauraTheme::AccentBlue
                                  : hovered ? BauraTheme::SurfaceRaised
                                            : BauraTheme::GlassDark;
    renderer.draw_rounded_rect(x, y, width, height, std::min(12, height / 2),
                               background, 235);
    renderer.draw_rounded_rect(x, y, width, height, std::min(12, height / 2),
                               BauraTheme::Border, 100);
    char shown_label[33];
    const size_t shown_length = std::min(label.size(), sizeof(shown_label) - 1);
    std::copy_n(label.data(), shown_length, shown_label);
    shown_label[shown_length] = '\0';
    const int text_x = x + std::max(
        8, (width - static_cast<int>(shown_length) * 6) / 2);
    renderer.draw_text(text_x, y + std::max(4, (height - 7) / 2),
                       shown_label, BauraTheme::TextPrimary);
}

void BauraButton::handle_event(EventType type, int mouse_x, int mouse_y) {
    hovered = visible && enabled && width >= 24 && height >= 20 &&
              contains(mouse_x, mouse_y);
    if (type != EventType::CLICK || !hovered) return;
    is_pressed = true;
    if (action != nullptr) action(action_context);
    is_pressed = false;
}

void BauraButton::set_action(void (*handler)(void *), void *context) {
    action = handler;
    action_context = context;
}
void BauraButton::set_label(const std::string &text) { label = text; }
bool BauraButton::pressed() const { return is_pressed; }

BauraWindow::BauraWindow(const std::string &window_title, int origin_x,
                         int origin_y, int window_width, int window_height)
    : BauraWidget(origin_x, origin_y, window_width, window_height),
      title(window_title),
      action_handler(nullptr),
      action_context(nullptr),
      focused(false),
      is_minimized(false),
      is_maximized(false),
      dragging(false),
      drag_offset_x(0),
      drag_offset_y(0),
      normal_bounds{origin_x, origin_y, window_width, window_height},
      work_area{0, 0, 0, 0},
      has_work_area(false) {}

BGERect BauraWindow::control_bounds(unsigned index) const {
    const int offsets[] = {86, 58, 30};
    return {x + width - offsets[index], y + 8, 24, 24};
}

void BauraWindow::draw(BGERenderer &renderer) {
    if (!visible || is_minimized ||
        !valid_window_bounds(x, y, width, height)) return;
    renderer.draw_rounded_rect(x + 2, y + 4, width, height, 16,
                               BauraTheme::Shadow, 120);
    renderer.draw_rounded_rect(x, y, width, height, 16,
                               BauraTheme::GlassDark, 245);
    renderer.draw_rounded_rect(x, y, width, 46, 16,
                               focused ? BauraTheme::SurfaceRaised
                                       : BauraTheme::Surface,
                               250);
    renderer.fill_rect(x, y + 34, width, 12,
                       focused ? BauraTheme::SurfaceRaised
                               : BauraTheme::Surface);
    renderer.draw_text(x + 18, y + 20, title.c_str(),
                       BauraTheme::TextPrimary);
    const uint32_t colors[3] = {
        BauraTheme::Warning, BauraTheme::Success, BauraTheme::Danger
    };
    const char *icons[3] = {
        "window-minimize", "window-maximize", "window-close"
    };
    for (unsigned index = 0; index < 3; ++index) {
        const BGERect control = control_bounds(index);
        renderer.draw_rounded_rect(control.x, control.y,
                                   control.width, control.height, 12,
                                   colors[index], 235);
        renderer.draw_icon(control.x + 5, control.y + 5, 14,
                           BauraTheme::TextPrimary, icons[index]);
    }
    for (BauraWidget *child : children) {
        if (child != nullptr) child->draw(renderer);
    }
}

void BauraWindow::handle_event(EventType type, int mouse_x, int mouse_y) {
    if (!visible || is_minimized || !enabled ||
        !valid_window_bounds(x, y, width, height)) return;
    if (type == EventType::RELEASE) {
        dragging = false;
        return;
    }
    if (type == EventType::DRAG && dragging && !is_maximized) {
        const int64_t next_x = static_cast<int64_t>(mouse_x) - drag_offset_x;
        const int64_t next_y = static_cast<int64_t>(mouse_y) - drag_offset_y;
        if (next_x >= INT_MIN && next_x <= INT_MAX &&
            next_y >= INT_MIN && next_y <= INT_MAX &&
            next_x + width <= INT_MAX && next_y + height <= INT_MAX) {
            x = static_cast<int>(next_x);
            y = static_cast<int>(next_y);
            normal_bounds = {x, y, width, height};
        }
        return;
    }
    if (type == EventType::CLICK) {
        dragging = false;
        for (unsigned index = 0; index < 3; ++index) {
            const BGERect control = control_bounds(index);
            if (mouse_x < control.x || mouse_y < control.y ||
                mouse_x >= control.x + control.width ||
                mouse_y >= control.y + control.height) continue;
            if (index == 0) is_minimized = true;
            if (index == 1) {
                if (is_maximized) {
                    restore();
                } else if (has_work_area) {
                    normal_bounds = {x, y, width, height};
                    x = work_area.x;
                    y = work_area.y;
                    width = work_area.width;
                    height = work_area.height;
                    is_maximized = true;
                }
            }
            if (index == 2) visible = false;
            if (action_handler != nullptr) {
                action_handler(static_cast<BauraWindowAction>(index),
                               action_context);
            }
            return;
        }
        if (mouse_x >= x &&
            static_cast<int64_t>(mouse_x) <
                static_cast<int64_t>(x) + width &&
            mouse_y >= y &&
            static_cast<int64_t>(mouse_y) < static_cast<int64_t>(y) + 46 &&
            !is_maximized) {
            dragging = true;
            drag_offset_x = static_cast<int>(
                static_cast<int64_t>(mouse_x) - x);
            drag_offset_y = static_cast<int>(
                static_cast<int64_t>(mouse_y) - y);
            return;
        }
    }
    for (auto child = children.rbegin(); child != children.rend(); ++child) {
        if (*child != nullptr && (*child)->visible &&
            (*child)->contains(mouse_x, mouse_y)) {
            (*child)->handle_event(type, mouse_x, mouse_y);
            return;
        }
    }
}

bool BauraWindow::add_widget(BauraWidget *widget) {
    if (widget == nullptr ||
        std::find(children.begin(), children.end(), widget) != children.end()) {
        return false;
    }
    try {
        children.push_back(widget);
    } catch (const std::bad_alloc &) {
        return false;
    }
    return true;
}

void BauraWindow::set_action_handler(BauraWindowActionHandler handler,
                                     void *context) {
    action_handler = handler;
    action_context = context;
}

void BauraWindow::set_focused(bool value) { focused = value; }
void BauraWindow::set_work_area(int origin_x, int origin_y, int area_width,
                                int area_height) {
    if (area_width < 120 || area_height < 72 ||
        !valid_widget_bounds(origin_x, origin_y, area_width, area_height)) {
        return;
    }
    work_area = {origin_x, origin_y, area_width, area_height};
    has_work_area = true;
}
void BauraWindow::restore() {
    if (is_maximized) {
        x = normal_bounds.x;
        y = normal_bounds.y;
        width = normal_bounds.width;
        height = normal_bounds.height;
        is_maximized = false;
    }
    is_minimized = false;
    visible = true;
}
bool BauraWindow::minimized() const { return is_minimized; }
bool BauraWindow::maximized() const { return is_maximized; }
bool BauraWindow::closed() const { return !visible; }
bool BauraWindow::is_dragging() const { return dragging; }
BGERect BauraWindow::bounds() const { return {x, y, width, height}; }
