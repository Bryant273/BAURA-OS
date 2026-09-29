
#include "BGE_Widgets.cpp"
#include "L4_TCB.h"

class TaskManagerApp : public BGEWindow {
    BGETable* proc_table;
    BGEGraph* cpu_graph; // Real-time line graph for CPU load

public:
    TaskManagerApp() {
        proc_table = new BGETable(10, 10, 600, 300, {"PID", "Nom", "CPU %", "RAM", "Etat"});
        cpu_graph = new BGEGraph(610, 10, 300, 300, "Charge CPU");
        add_widget(proc_table);
        add_widget(cpu_graph);
    }

    void update_loop() {
        // 1. Get global snapshot via API 901
        SystemSnapshot snap = syscall_api_sys_get_snapshot();

        // 2. Update Graph
        cpu_graph->add_point(snap.cpu_load);

        // 3. Update Process Table
        auto tasks = syscall_api_proc_get_all();
        proc_table->clear();
        for (auto& t : tasks) {
            // Calculate actual CPU usage based on TCB time_slice
            float usage = (float)t.runtime / t.total_time * 100;
            proc_table->add_row({t.pid, t.name, usage, t.mem_usage, t.state});
        }
    }

    void on_kill_selected() {
        uint32_t pid = proc_table->get_selected_pid();
        // API 280: Send SIGKILL
        syscall_api_sig_send(pid, SIGKILL);
    }
};
