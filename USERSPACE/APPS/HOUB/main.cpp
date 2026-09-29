
#include "BGE_Widgets.cpp"
#include "Thorium_Engine.h"

class HOUBBrowser : public BGEWindow {
    BGEInput* address_bar;
    BGEWebCanvas* web_view;

public:
    HOUBBrowser() {
        this->title = "HOUB Browser";
        address_bar = new BGEInput(150, 10, 900, 40, "Search with DuckDuckGo...");
        address_bar->set_style(STYLE_CAPSULE);
        web_view = new BGEWebCanvas(10, 60, 1180, 700);
        add_widget(address_bar);
        add_widget(web_view);
    }

    void on_navigate() {
        std::string url = address_bar->get_text();
        thorium_load_url(web_view->get_handle(), url);
    }
};
