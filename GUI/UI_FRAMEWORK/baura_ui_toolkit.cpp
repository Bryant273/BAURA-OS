
#include <string>
#include <vector>

class BauraWidget {
public:
    virtual void draw() = 0;
    virtual void handle_event(int event_id) = 0;
    int x, y, width, height;
};

class BauraButton : public BauraWidget {
    std::string label;
public:
    BauraButton(std::string text) : label(text) {}
    void draw() override {
        // Render button with optimized vector graphics
    }
    void handle_event(int event_id) override {
        // Handle click
    }
};

class BauraWindow : public BauraWidget {
    std::vector<BauraWidget*> children;
public:
    void add_widget(BauraWidget* w) { children.push_back(w); }
    void draw() override {
        for(auto w : children) w->draw();
    }
};
