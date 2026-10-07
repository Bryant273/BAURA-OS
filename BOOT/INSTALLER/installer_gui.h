#ifndef BAURA_INSTALLER_GUI_H
#define BAURA_INSTALLER_GUI_H

#include "../../GUI/BGE/renderer.h"

#include <cstdint>

enum class BauraInstallStage : uint32_t {
    Welcome,
    HardwareScan,
    Partition,
    Format,
    CopySystem,
    Complete,
    Failed
};

typedef int (*BauraInstallerStageAction)(void *context,
                                         BauraInstallStage stage);

class BauraInstaller {
public:
    BauraInstaller();
    void set_backend(BauraInstallerStageAction action, void *context);
    bool run_stage();
    bool retry();
    bool handle_click(int x, int y, uint32_t screen_width,
                      uint32_t screen_height);
    void render(BGERenderer &renderer);
    BauraInstallStage stage() const;
    int last_error() const;

private:
    BauraInstallStage current_stage;
    BauraInstallStage failed_stage;
    BauraInstallerStageAction action;
    void *action_context;
    int error;
};

#endif
