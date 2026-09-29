
#include "BAURA_SCHEDULER.h"
#include <vector>
#include <queue>

// Multi-Level Feedback Queue (MLFQ) for high responsiveness (like Windows/macOS)
class BauraProScheduler {
    struct Queue {
        uint32_t priority;
        uint32_t time_slice;
        std::queue<uint64_t> tasks;
    };
    std::vector<Queue> queues;

public:
    BauraProScheduler() {
        // Initialize 4 levels of priority: Real-time, Interactive, Normal, Batch
        queues.push_back({0, 10, {}});  // Real-time (10ms slice)
        queues.push_back({1, 40, {}});  // Interactive (40ms slice)
        queues.push_back({2, 100, {}}); // Normal (100ms slice)
        queues.push_back({3, 500, {}}); // Batch (500ms slice)
    }

    uint64_t schedule_next() {
        for (auto& q : queues) {
            if (!q.tasks.empty()) {
                uint64_t tid = q.tasks.front();
                q.tasks.pop();
                return tid;
            }
        }
        return 0;
    }

    void handle_quantum_expire(uint64_t tid, uint32_t current_q) {
        // Lower priority if task uses whole slice (prevents CPU hogging)
        if (current_q < 3) {
            queues[current_q + 1].tasks.push(tid);
        } else {
            queues[3].tasks.push(tid);
        }
    }
};
