#ifndef BAURA_SECURITY_RUST_H
#define BAURA_SECURITY_RUST_H

#include <stdint.h>

#define CAP_READ (1u << 0)
#define CAP_WRITE (1u << 1)
#define CAP_EXEC (1u << 2)
#define CAP_GRANT (1u << 3)

typedef struct { uint64_t object_id; uint32_t rights; } BauraCapability;

#ifdef __cplusplus
extern "C" {
#endif
int sec_create_cspace(uint64_t task_id);
int sec_destroy_cspace(uint64_t task_id);
int sec_mint_capability(uint64_t task_id, uint32_t slot, uint64_t object_id, uint32_t rights);
int sec_validate_capability(uint64_t task_id, uint32_t slot, uint32_t required_right);
int sec_revoke_capability(uint64_t task_id, uint32_t slot);
int sec_delegate_capability(uint64_t src_task, uint32_t src_slot, uint64_t dst_task, uint32_t dst_slot, uint32_t requested_rights);
#ifdef __cplusplus
}
#endif

#endif
