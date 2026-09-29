#include "baura_process_manager.h"

typedef struct { BauraProcessInfo info; uint8_t active; } ProcessEntry;
static ProcessEntry processes[BAURA_PROCESS_CAPACITY];
static uint64_t next_process_id = 1;
static uint32_t process_count;

static ProcessEntry *find_process(uint64_t id) {
    uint32_t index;
    for (index = 0; index < BAURA_PROCESS_CAPACITY; ++index) if (processes[index].active && processes[index].info.id == id) return &processes[index];
    return 0;
}

void process_manager_init(void) {
    uint32_t index;
    for (index = 0; index < BAURA_PROCESS_CAPACITY; ++index) processes[index].active = 0;
    next_process_id = 1; process_count = 0;
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
        processes[index].active = 1; ++process_count; *process_id = processes[index].info.id; return BAURA_OK;
    }
    return BAURA_ERR_CAPACITY;
}

BauraStatus baura_process_set_state(uint64_t process_id, BauraProcessState state) {
    ProcessEntry *process = find_process(process_id);
    if (process == 0) return BAURA_ERR_NOT_FOUND;
    if ((uint32_t)state > (uint32_t)BAURA_PROCESS_TERMINATED) return BAURA_ERR_INVALID_ARGUMENT;
    if (process->info.state == BAURA_PROCESS_TERMINATED && state != BAURA_PROCESS_TERMINATED) return BAURA_ERR_INVALID_STATE;
    if (state == process->info.state) return BAURA_OK;
    if (process->info.state == BAURA_PROCESS_READY && state != BAURA_PROCESS_RUNNING && state != BAURA_PROCESS_TERMINATED) return BAURA_ERR_INVALID_STATE;
    if (process->info.state == BAURA_PROCESS_RUNNING && state != BAURA_PROCESS_READY && state != BAURA_PROCESS_BLOCKED && state != BAURA_PROCESS_TERMINATED) return BAURA_ERR_INVALID_STATE;
    if (process->info.state == BAURA_PROCESS_BLOCKED && state != BAURA_PROCESS_READY && state != BAURA_PROCESS_TERMINATED) return BAURA_ERR_INVALID_STATE;
    process->info.state = state; return BAURA_OK;
}

BauraStatus baura_process_query(uint64_t process_id, BauraProcessInfo *info) {
    ProcessEntry *process = find_process(process_id);
    if (process == 0) return BAURA_ERR_NOT_FOUND;
    if (info == 0) return BAURA_ERR_INVALID_ARGUMENT;
    *info = process->info; return BAURA_OK;
}

uint32_t baura_process_count(void) { return process_count; }
