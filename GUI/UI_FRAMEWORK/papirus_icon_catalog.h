#ifndef BAURA_PAPIRUS_ICON_CATALOG_H
#define BAURA_PAPIRUS_ICON_CATALOG_H

#include <cstddef>
#include <cstdint>

struct BauraIconEntry {
    const char *id;
    const char *papirus_name;
    uint32_t tint;
};

const BauraIconEntry *baura_icon_catalog(size_t *count);
const BauraIconEntry *baura_icon_find(const char *id);

#endif
