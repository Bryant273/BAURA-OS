
#include "BGE_Widgets.cpp"
#include "BAURA_VFS.h"

class FichiersApp : public BGEWindow {
    // State Management
    std::string current_path = "PC/C:/";
    std::vector<std::string> history;

    // UI Components
    BGEList* nav_panel;
    BGEGrid* content_panel;
    BGEDetailsPane* info_panel;

public:
    FichiersApp() {
        // Complex UI Setup (Symmetry with Windows 11)
        setup_ui();
        refresh_view(current_path);
    }

    void setup_ui() {
        nav_panel = new BGEList(10, 50, 200, 540, "Navigation");
        content_panel = new BGEGrid(220, 50, 550, 540);
        info_panel = new BGEDetailsPane(780, 50, 210, 540, "Détails");

        // Add system shortcuts to nav_panel
        nav_panel->add_item("🏠 Accueil", &this->go_home);
        nav_//panel->add_item("💾 Disque C:", &this->go_root);
        // ... [Adding all virtual drives]
    }

    void refresh_view(std::string path) {
        current_path = path;
        content_panel->clear();

        // Detailed VFS readout
        auto entries = syscall_vfs_read_dir(path.c_str());
        for (auto& e : entries) {
            // Logic to assign icons based on file extension (.aesb, .txt, .bin)
            uint32_t icon_id = icon_manager_get_id(e.extension);
            content_panel->add_icon(e.name, icon_id, &this->on_item_clicked);
        }
    }

    void on_item_clicked(FileEntry e) {
        if (e.is_dir) {
            refresh_view(current_path + "/" + e.name);
        } else {
            // Update the Info Panel with metadata from BAURAFS Inodes
            info_//panel->set_property("Size", e.size);
            info_panel->set_property("Owner", e.owner);
            info_panel->set_property("Permissions", e.perms);
        }
    }
};
