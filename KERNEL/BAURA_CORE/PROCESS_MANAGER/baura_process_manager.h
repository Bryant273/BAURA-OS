#ifndef BAURA_PROCESS_MANAGER_H
#define BAURA_PROCESS_MANAGER_H

#include "../baura_core_types.h"

#define BAURA_PROCESS_CAPACITY 64u

typedef enum { BAURA_PROCESS_READY, BAURA_PROCESS_RUNNING, BAURA_PROCESS_BLOCKED, BAURA_PROCESS_TERMINATED } BauraProcessState;
typedef struct { uint64_t id; uint64_t parent_id; BauraProcessState state; char name[BAURA_NAME_MAX]; } BauraProcessInfo;

void process_manager_init(void);
BauraStatus baura_process_create(const char *name, uint64_t parent_id, uint64_t *process_id);
BauraStatus baura_process_set_state(uint64_t process_id, BauraProcessState state);
BauraStatus baura_process_query(uint64_t process_id, BauraProcessInfo *info);
uint32_t baura_process_count(void);

#endif
