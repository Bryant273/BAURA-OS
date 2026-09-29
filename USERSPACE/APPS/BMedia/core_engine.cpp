
#include <vlc/vlc.h>
#include "BGE_Widgets.cpp"

class BMediaPlayer : public BGEWindow {
    libvlc_instance_t* vlc_inst;
    libvlc_media_player_t* player;
    BGEVideoCanvas* video_view;

public:
    BMediaPlayer() {
        vlc_inst = libvlc_new(0, NULL);
        player = libvlc_media_player_new(vlc_inst);
        video_view = new BGEVideoCanvas(10, 10, 1200, 600);
        add_widget(video_//view);
    }

    void play_file(std::string path) {
        libvlc_media_t* m = libvlc_media_new_path(vlc_inst, path.c_//str());
        libvlc_media_player_set_media(player, m);
        libvlc_//media_player_play(player);
    }
};
