#include "papirus_icon_catalog.h"

#include "baura_theme.h"

#include <cstring>

static const BauraIconEntry icons[] = {
    {"folder", "folder", BauraTheme::Warning},
    {"folder-home", "folder-home", BauraTheme::AccentBlue},
    {"folder-downloads", "folder-download", BauraTheme::AccentCyan},
    {"folder-documents", "folder-documents", BauraTheme::AccentPurple},
    {"folder-pictures", "folder-pictures", BauraTheme::Danger},
    {"file", "text-x-generic", BauraTheme::TextSecondary},
    {"file-text", "text-plain", BauraTheme::TextPrimary},
    {"file-image", "image-x-generic", BauraTheme::AccentPurple},
    {"file-audio", "audio-x-generic", BauraTheme::AccentCyan},
    {"file-video", "video-x-generic", BauraTheme::Danger},
    {"application", "application-x-executable", BauraTheme::AccentBlue},
    {"settings", "preferences-system", BauraTheme::TextSecondary},
    {"network", "network-wireless", BauraTheme::AccentCyan},
    {"battery", "battery", BauraTheme::Success},
    {"search", "system-search", BauraTheme::TextSecondary},
    {"notifications", "preferences-system-notifications", BauraTheme::Warning},
    {"media-play", "media-playback-start", BauraTheme::Success},
    {"media-pause", "media-playback-pause", BauraTheme::Warning},
    {"window-minimize", "window-minimize", BauraTheme::TextSecondary},
    {"window-maximize", "window-maximize", BauraTheme::TextSecondary},
    {"window-close", "window-close", BauraTheme::Danger},
    {"install", "system-software-install", BauraTheme::AccentBlue},
    {"user", "user-identity", BauraTheme::AccentCyan},
    {"lock", "system-lock-screen", BauraTheme::Warning}
};

const BauraIconEntry *baura_icon_catalog(size_t *count) {
    if (count != nullptr) *count = sizeof(icons) / sizeof(icons[0]);
    return icons;
}

const BauraIconEntry *baura_icon_find(const char *id) {
    if (id == nullptr) return nullptr;
    for (const BauraIconEntry &icon : icons) {
        if (std::strcmp(icon.id, id) == 0) return &icon;
    }
    return nullptr;
}
