
#ifndef BAURA_TASK_MANAGER_H
#define BAURA_TASK_MANAGER_H

#include "BAURA_GUI_FFI.h"
#include "BAURA_THEME.h"
#include <string>
#include <vector>

struct ProcessInfo {
    uint64_t pid;
    std::string name;
    float cpu_usage;
    float ram_usage;
    float disk_io;
    float net_io;
    float temperature;
    uint32_t capabilities_mask;
    std::string status;
};

class BauraTaskManager {
public:
    BauraTaskManager();
    void render_main_window();
    void kill_process(uint64_t pid);

private:
    void draw_process_table();
    void draw_performance_graphs();
    void draw_thermal_map();

    std::vector<ProcessInfo> current_processes;
};
#endif
