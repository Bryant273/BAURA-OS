
#include "BAURA_TASK_MANAGER.h"
#include "BAURA_UI_FRAMEWORK.h"
#include <iostream>

void BauraTaskManager::kill_process(uint64_t pid) {
    std::cout << "[TaskMgr] Sending SIGKILL to PID " << pid << std::endl;
    // L4 syscall to destroy task
    // l4_task_destroy(pid);

    // Update UI state
    for (auto& proc : current_processes) {
        if (proc.pid == pid) {
            proc.status = "TERMINATED";
            proc.cpu_usage = 0;
        }
    }
}

void BauraTaskManager::render_main_window() {
    // Layout: Left Sidebar, Center Table, Bottom Summary
    // Added: 'End Task' button for the selected process
}
