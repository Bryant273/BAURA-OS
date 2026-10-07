#ifndef BAURA_SECURITY_H
#define BAURA_SECURITY_H

#include "../baura_core_types.h"
#include "BAURA_SECURITY_RUST.h"

typedef BauraStatus (*BauraSecurityAuthorityCheck)(
    void *context, uint64_t subject_id, const char *capability_name);

void security_init(void);
BauraStatus baura_security_set_authority_check(
    BauraSecurityAuthorityCheck check, void *context);
BauraStatus baura_security_authorize(
    uint64_t subject_id, const char *capability_name);

#endif
