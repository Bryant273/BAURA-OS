#include "installer_gui.h"

#include "../../GUI/UI_FRAMEWORK/baura_theme.h"

#include <climits>

BauraInstaller::BauraInstaller()
    : current_stage(BauraInstallStage::Welcome),
      failed_stage(BauraInstallStage::HardwareScan),
      action(nullptr),
      action_context(nullptr),
      error(0) {}

void BauraInstaller::set_backend(BauraInstallerStageAction handler,
                                 void *context) {
    action = handler;
    action_context = context;
}

bool BauraInstaller::run_stage() {
    if (current_stage == BauraInstallStage::Complete ||
        current_stage == BauraInstallStage::Failed || action == nullptr) {
        return false;
    }
    const BauraInstallStage work =
        current_stage == BauraInstallStage::Welcome
            ? BauraInstallStage::HardwareScan
            : current_stage;
    const int result = action(action_context, work);
    if (result != 0) {
        failed_stage = work;
        error = result;
        current_stage = BauraInstallStage::Failed;
        return false;
    }
    error = 0;
    switch (work) {
    case BauraInstallStage::HardwareScan:
        current_stage = BauraInstallStage::Partition;
        break;
    case BauraInstallStage::Partition:
        current_stage = BauraInstallStage::Format;
        break;
    case BauraInstallStage::Format:
        current_stage = BauraInstallStage::CopySystem;
        break;
    case BauraInstallStage::CopySystem:
        current_stage = BauraInstallStage::Complete;
        break;
    default:
        return false;
    }
    return true;
}

bool BauraInstaller::retry() {
    if (current_stage != BauraInstallStage::Failed) return false;
    current_stage = failed_stage;
    error = 0;
    return run_stage();
}

bool BauraInstaller::handle_click(int mouse_x, int mouse_y,
                                  uint32_t screen_width,
                                  uint32_t screen_height) {
    if (screen_width > static_cast<uint32_t>(INT_MAX) ||
        screen_height > static_cast<uint32_t>(INT_MAX) ||
        screen_width < 620 || screen_height < 420) return false;
    const int x = (static_cast<int>(screen_width) - 620) / 2;
    const int y = (static_cast<int>(screen_height) - 420) / 2;
    const int button_x = x + 620 - 194;
    const int button_y = y + 344;
    if (mouse_x < button_x || mouse_x >= button_x + 160 ||
        mouse_y < button_y || mouse_y >= button_y + 44) return false;
    if (current_stage == BauraInstallStage::Failed) {
        retry();
        return true;
    }
    if (current_stage == BauraInstallStage::Complete) return true;
    run_stage();
    return true;
}

BauraInstallStage BauraInstaller::stage() const { return current_stage; }
int BauraInstaller::last_error() const { return error; }

void BauraInstaller::render(BGERenderer &renderer) {
    if (!renderer.valid()) return;
    const BGEFramebuffer &fb = renderer.framebuffer();
    const int panel_width = 620;
    const int panel_height = 420;
    const int x = (static_cast<int>(fb.width) - panel_width) / 2;
    const int y = (static_cast<int>(fb.height) - panel_height) / 2;
    renderer.draw_gradient_rect(0, 0, static_cast<int>(fb.width),
                                static_cast<int>(fb.height), 0xFF182944u,
                                0xFF182137u);
    renderer.draw_rounded_rect(x, y, panel_width, panel_height, 22,
                               BauraTheme::GlassDark, 245);
    renderer.draw_text(x + 34, y + 42, "BAURA OS", BauraTheme::TextPrimary, 2);
    renderer.draw_text(x + 34, y + 76, "A simpler, calmer desktop",
                       BauraTheme::TextSecondary);
    static const char *labels[] = {
        "Welcome", "Hardware", "Partition", "Format", "Install", "Finish"
    };
    const uint32_t stage_index =
        current_stage == BauraInstallStage::Welcome ? 0u :
        current_stage == BauraInstallStage::HardwareScan ? 1u :
        current_stage == BauraInstallStage::Partition ? 2u :
        current_stage == BauraInstallStage::Format ? 3u :
        current_stage == BauraInstallStage::CopySystem ? 4u :
        current_stage == BauraInstallStage::Complete ? 5u : 1u;
    renderer.draw_rounded_rect(x + 34, y + 120, panel_width - 68, 8, 4,
                               BauraTheme::SurfaceRaised);
    renderer.draw_rounded_rect(x + 34, y + 120,
                               (panel_width - 68) *
                                   static_cast<int>(stage_index + 1u) / 6,
                               8, 4, BauraTheme::AccentBlue);
    renderer.draw_text(x + 34, y + 158, labels[stage_index],
                       BauraTheme::AccentCyan, 2);
    const char *detail = current_stage == BauraInstallStage::Failed
                             ? "The operation failed. Check the backend and retry."
                         : current_stage == BauraInstallStage::Complete
                             ? "Installation finished. You can restart into BAURA OS."
                             : "Continue when you are ready. Your selected action is delegated to the installer backend.";
    renderer.draw_text(x + 34, y + 198, detail,
                       current_stage == BauraInstallStage::Failed
                           ? BauraTheme::Danger
                           : BauraTheme::TextSecondary);
    renderer.draw_rounded_rect(x + panel_width - 194, y + 344, 160, 44, 14,
                               current_stage == BauraInstallStage::Complete
                                   ? BauraTheme::Success
                                   : BauraTheme::AccentBlue);
    renderer.draw_text(x + panel_width - 155, y + 362,
                       current_stage == BauraInstallStage::Failed
                           ? "Retry"
                       : current_stage == BauraInstallStage::Complete
                           ? "Done"
                           : "Continue",
                       BauraTheme::TextPrimary);
}
