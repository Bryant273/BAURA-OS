/* Exemple (proposition) : anneau SPSC (un producteur, un consommateur) pour la
 * mémoire partagée entre le Core et un pilote isolé. Capacité = puissance de 2.
 * Un seul écrivain par indice ; synchronisation par acquire/release.
 * Voir 03_MODELE_DE_CONCURRENCE.md. */
#ifndef BAURA_SPSC_RING_H
#define BAURA_SPSC_RING_H
#include <stdint.h>

#define BAURA_RING_SLOTS 64u /* puissance de 2 */
#define BAURA_RING_SLOT_BYTES 64u

typedef struct {
    uint32_t head; /* écrit par le producteur seulement */
    uint32_t pad1[15];
    uint32_t tail; /* écrit par le consommateur seulement */
    uint32_t pad2[15];
    uint8_t slot[BAURA_RING_SLOTS][BAURA_RING_SLOT_BYTES];
} BauraSpscRing;

static inline int baura_ring_push(BauraSpscRing *r, const void *src)
{
    uint32_t h = __atomic_load_n(&r->head, __ATOMIC_RELAXED);
    uint32_t t = __atomic_load_n(&r->tail, __ATOMIC_ACQUIRE);
    if (h - t >= BAURA_RING_SLOTS) return 0; /* plein */
    __builtin_memcpy(r->slot[h & (BAURA_RING_SLOTS - 1u)], src,
                     BAURA_RING_SLOT_BYTES);
    __atomic_store_n(&r->head, h + 1u, __ATOMIC_RELEASE);
    return 1;
}

static inline int baura_ring_pop(BauraSpscRing *r, void *dst)
{
    uint32_t t = __atomic_load_n(&r->tail, __ATOMIC_RELAXED);
    uint32_t h = __atomic_load_n(&r->head, __ATOMIC_ACQUIRE);
    if (h == t) return 0; /* vide */
    __builtin_memcpy(dst, r->slot[t & (BAURA_RING_SLOTS - 1u)],
                     BAURA_RING_SLOT_BYTES);
    __atomic_store_n(&r->tail, t + 1u, __ATOMIC_RELEASE);
    return 1;
}
#endif
