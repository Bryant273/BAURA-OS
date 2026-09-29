 #ifndef BAURA_UI_FRAMEWORK_H
#define BAURA_UI_FRAMEWORK_H
#include "BAURA_DISPLAY_SERVER.h"
#include "BAURA_THEME.h"
#include <string>
#include <vector>

enum class EventType { CLICK, HOVER, DRAG, RESIZE };

class BauraWidget {
public:
    int x, y, width, height;
    bool visible = true;
    virtual void draw() = 0;
    virtual void handle_event(EventType type, int mx, int my) = 0;
};

class WindowControl {
public:
    enum class Type { MINIMIZE, MAXIMIZE, CLOSE };
    Type type;
    int x, y, size = 12;
    void draw() { /* Render based on type */ }
    bool is_clicked(int mx, int my) { return (mx >= x && mx <= x + size && my >= y && my <= y + size); }
};

class BauraWindow : public BauraWidget {
public:
    std::string title;
    uint32_t surface_id;
    std::vector<BauraWidget*> children;
    WindowControl btn_min, btn_max, btn_close;

    BauraWindow(std::string t, int w, int h) : title(t) {
        width = w; height = h;
        btn_close.x = w - 15; btn_close.y = 5; btn_close.type = WindowControl::Type::CLOSE;
        btn_max.x = w - 30;   btn_max.y = 5;   btn_max.type = WindowControl::Type::MAXIMIZE;
        btn_min.x = w - 45;   btn_min.y = 5;   btn_min.type = WindowControl::Type::MINIMIZE;
    }

    void draw() override {
        // Draw Glassy Border, Title Bar and (_, o, x) buttons
        btn_min.draw(); btn_max.draw(); btn_close.draw();
        for(auto w : children) w->draw();
    }

    void handle_event(EventType type, int mx, int my) override {
        if (type == EventType::CLICK) {
            if (btn_close.is_clicked(mx, my)) { /* close */ }
            if (btn_max.is_clicked(mx, my)) { /* maximize */ }
            if (btn_min.is_clicked(mx, my)) { /* minimize */ }
        }
    }
};
#endif