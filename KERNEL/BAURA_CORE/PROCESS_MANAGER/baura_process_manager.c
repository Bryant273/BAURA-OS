#include "baura_process_manager.h"

typedef struct {
    BauraProcessInfo info;
    uint64_t task_id;
    uint8_t kernel_managed;
    uint8_t active;
} ProcessEntry;
static ProcessEntry processes[BAURA_PROCESS_CAPACITY];
static uint64_t next_process_id = 1;
static uint32_t process_count;
static BauraProcessBackend process_backend;

static ProcessEntry *find_process(uint64_t id) {
    uint32_t index;
    for (index = 0; index < BAURA_PROCESS_CAPACITY; ++index) if (processes[index].active && processes[index].info.id == id) return &processes[index];
    return 0;
}

void process_manager_init(void) {
    uint32_t index;
    for (index = 0; index < BAURA_PROCESS_CAPACITY; ++index) {
        processes[index].active = 0;
        processes[index].kernel_managed = 0;
        processes[index].task_id = 0;
    }
    next_process_id = 1; process_count = 0;
}

BauraStatus baura_process_set_backend(const BauraProcessBackend *backend) {
    uint32_t index;
    if (backend != 0 &&
        (backend->start == 0 || backend->set_state == 0 || backend->terminate == 0)) {
        return BAURA_ERR_INVALID_ARGUMENT;
    }
    for (index = 0; index < BAURA_PROCESS_CAPACITY; ++index) {
        if (processes[index].active && processes[index].kernel_managed) {
            return BAURA_ERR_INVALID_STATE;
        }
    }
    if (backend == 0) {
        process_backend.start = 0;
        process_backend.set_state = 0;
        process_backend.terminate = 0;
        process_backend.context = 0;
    } else {
        process_backend = *backend;
    }
    return BAURA_OK;
}

BauraStatus baura_process_create(const char *name, uint64_t parent_id, uint64_t *process_id) {
    uint32_t index;
    uint32_t length = 0;
    if (name == 0 || process_id == 0 || name[0] == '\0') return BAURA_ERR_INVALID_ARGUMENT;
    while (length < BAURA_NAME_MAX && name[length] != '\0') ++length;
    if (length == BAURA_NAME_MAX) return BAURA_ERR_LIMIT;
    if (parent_id != 0 && (find_process(parent_id) == 0 || find_process(parent_id)->info.state == BAURA_PROCESS_TERMINATED)) return BAURA_ERR_NOT_FOUND;
    for (index = 0; index < BAURA_PROCESS_CAPACITY; ++index) if (!processes[index].active) {
        uint32_t char_index;
        processes[index].info.id = next_process_id++;
        if (next_process_id == 0) next_process_id = 1;
        processes[index].info.parent_id = parent_id;
        processes[index].info.state = BAURA_PROCESS_READY;
        for (char_index = 0; char_index < length; ++char_index) processes[index].info.name[char_index] = name[char_index];
        processes[index].info.name[length] = '\0';
        processes[index].task_id = 0;
        processes[index].kernel_managed = 0;
        processes[index].active = 1; ++process_count; *process_id = processes[index].info.id; return BAURA_OK;
    }
    return BAURA_ERR_CAPACITY;
}

BauraStatus baura_process_launch(const char *name, const char *image_path,
                                 const char *arguments, uint64_t parent_id,
                                 uint64_t *process_id) {
    uint32_t index;
    uint32_t length = 0;
    uint64_t parent_task_id = 0;
    uint64_t task_id = 0;
    BauraStatus status;
    if (name == 0 || image_path == 0 || process_id == 0 ||
        name[0] == '\0' || image_path[0] == '\0') {
        return BAURA_ERR_INVALID_ARGUMENT;
    }
    if (process_backend.start == 0 || process_backend.set_state == 0 ||
        process_backend.terminate == 0) {
        return BAURA_ERR_INVALID_STATE;
    }
    while (length < BAURA_NAME_MAX && name[length] != '\0') ++length;
    if (length == 0 || length == BAURA_NAME_MAX) return BAURA_ERR_LIMIT;
    if (parent_id != 0) {
        ProcessEntry *parent = find_process(parent_id);
        if (parent == 0 || parent->info.state == BAURA_PROCESS_TERMINATED) {
            return BAURA_ERR_NOT_FOUND;
        }
        if (!parent->kernel_managed) return BAURA_ERR_INVALID_STATE;
        parent_task_id = parent->task_id;
    }
    for (index = 0; index < BAURA_PROCESS_CAPACITY; ++index) {
        if (!processes[index].active) break;
    }
    if (index == BAURA_PROCESS_CAPACITY) return BAURA_ERR_CAPACITY;

    status = process_backend.start(process_backend.context, name, image_path,
                                   arguments == 0 ? "" : arguments,
                                   parent_task_id, &task_id);
    if (status != BAURA_OK) return status;
    if (task_id == 0) return BAURA_ERR_INVALID_STATE;

    processes[index].info.id = next_process_id++;
    if (next_process_id == 0) next_process_id = 1;
    processes[index].info.parent_id = parent_id;
    processes[index].info.state = BAURA_PROCESS_RUNNING;
    for (uint32_t char_index = 0; char_index < length; ++char_index) {
        processes[index].info.name[char_index] = name[char_index];
    }
    processes[index].info.name[length] = '\0';
    processes[index].task_id = task_id;
    processes[index].kernel_managed = 1;
    processes[index].active = 1;
    ++process_count;
    *process_id = processes[index].info.id;
    return BAURA_OK;
}

BauraStatus baura_process_set_state(uint64_t process_id, BauraProcessState state) {
    ProcessEntry *process = find_process(process_id);
    BauraStatus status;
    if (process == 0) return BAURA_ERR_NOT_FOUND;
    if ((uint32_t)state > (uint32_t)BAURA_PROCESS_TERMINATED) return BAURA_ERR_INVALID_ARGUMENT;
    if (process->info.state == BAURA_PROCESS_TERMINATED && state != BAURA_PROCESS_TERMINATED) return BAURA_ERR_INVALID_STATE;
    if (state == process->info.state) return BAURA_OK;
    if (process->info.state == BAURA_PROCESS_READY && state != BAURA_PROCESS_RUNNING && state != BAURA_PROCESS_TERMINATED) return BAURA_ERR_INVALID_STATE;
    if (process->info.state == BAURA_PROCESS_RUNNING && state != BAURA_PROCESS_READY && state != BAURA_PROCESS_BLOCKED && state != BAURA_PROCESS_TERMINATED) return BAURA_ERR_INVALID_STATE;
    if (process->info.state == BAURA_PROCESS_BLOCKED && state != BAURA_PROCESS_READY && state != BAURA_PROCESS_TERMINATED) return BAURA_ERR_INVALID_STATE;
    if (process->kernel_managed) {
        if (state == BAURA_PROCESS_TERMINATED) {
            status = process_backend.terminate(process_backend.context, process->task_id);
            if (status == BAURA_OK) {
                process->task_id = 0;
                process->kernel_managed = 0;
            }
        } else {
            status = process_backend.set_state(process_backend.context, process->task_id, state);
        }
        if (status != BAURA_OK) return status;
    }
    process->info.state = state; return BAURA_OK;
}

BauraStatus baura_process_query(uint64_t process_id, BauraProcessInfo *info) {
    ProcessEntry *process = find_process(process_id);
    if (process == 0) return BAURA_ERR_NOT_FOUND;
    if (info == 0) return BAURA_ERR_INVALID_ARGUMENT;
    *info = process->info; return BAURA_OK;
}

uint32_t baura_process_count(void) { return process_count; }
