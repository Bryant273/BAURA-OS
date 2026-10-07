#ifndef BAURA_UI_FRAMEWORK_H
#define BAURA_UI_FRAMEWORK_H

#include "../BGE/renderer.h"
#include "baura_theme.h"

#include <cstdint>
#include <string>
#include <vector>

enum class EventType { CLICK, HOVER, DRAG, RESIZE, RELEASE };
enum class BauraWindowAction { MINIMIZE, MAXIMIZE, CLOSE };
typedef void (*BauraWindowActionHandler)(BauraWindowAction action,
                                         void *context);

class BauraWidget {
public:
    BauraWidget(int x, int y, int width, int height);
    virtual ~BauraWidget() = default;
    virtual void draw(BGERenderer &renderer) = 0;
    virtual void handle_event(EventType type, int mouse_x, int mouse_y);
    bool contains(int px, int py) const;

    int x;
    int y;
    int width;
    int height;
    bool visible;
    bool enabled;
};

class BauraButton : public BauraWidget {
public:
    BauraButton(int x, int y, int width, int height,
                const std::string &label);
    void draw(BGERenderer &renderer) override;
    void handle_event(EventType type, int mouse_x, int mouse_y) override;
    void set_action(void (*action)(void *context), void *context);
    void set_label(const std::string &label);
    bool pressed() const;

private:
    std::string label;
    bool hovered;
    bool is_pressed;
    void (*action)(void *context);
    void *action_context;
};

class BauraWindow : public BauraWidget {
public:
    BauraWindow(const std::string &title, int x, int y, int width, int height);
    void draw(BGERenderer &renderer) override;
    void handle_event(EventType type, int mouse_x, int mouse_y) override;
    bool add_widget(BauraWidget *widget);
    void set_action_handler(BauraWindowActionHandler handler, void *context);
    void set_focused(bool focused);
    void set_work_area(int x, int y, int width, int height);
    void restore();
    bool minimized() const;
    bool maximized() const;
    bool closed() const;
    bool is_dragging() const;
    BGERect bounds() const;

private:
    std::string title;
    std::vector<BauraWidget *> children;
    BauraWindowActionHandler action_handler;
    void *action_context;
    bool focused;
    bool is_minimized;
    bool is_maximized;
    bool dragging;
    int drag_offset_x;
    int drag_offset_y;
    BGERect normal_bounds;
    BGERect work_area;
    bool has_work_area;
    BGERect control_bounds(unsigned index) const;
};

#endif
