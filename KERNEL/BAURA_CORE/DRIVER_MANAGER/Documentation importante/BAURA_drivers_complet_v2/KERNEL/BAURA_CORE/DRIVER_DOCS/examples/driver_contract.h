/* Exemple (proposition) : structures du contrat pilote. N'existe pas encore
 * dans le Core. Voir 04_INTERFACES_DE_CLASSE.md, 05_MODELE_DE_SECURITE.md et
 * ../DRIVER_DEVELOPMENT_GUIDE.md §3 et §5. */
#ifndef BAURA_DRIVER_CONTRACT_H
#define BAURA_DRIVER_CONTRACT_H
#include <stddef.h>
#include <stdint.h>

/* Accès matériel donné à un pilote : le pilote n'appelle jamais seL4 lui-même. */
typedef struct BauraHwAccess {
    void *ctx;
    uint32_t (*io_in)(void *ctx, uint16_t port, unsigned width_bytes);
    int (*io_out)(void *ctx, uint16_t port, unsigned width_bytes, uint32_t v);
    /* BAR mappé (CacheDisabled sauf mention contraire) ; NULL si refusé. */
    volatile void *(*map_bar)(void *ctx, unsigned bar, size_t *len);
    /* Tampon DMA contigu : adresse virtuelle ET adresse bus. */
    int (*dma_alloc)(void *ctx, size_t len, void **virt, uint64_t *bus);
    void (*dma_free)(void *ctx, void *virt);
    /* Attend une IRQ ; timeout_us==0 : non bloquant. 0 = reçue, 1 = délai. */
    int (*irq_wait)(void *ctx, uint32_t timeout_us);
    void (*irq_ack)(void *ctx);
    void (*delay_us)(void *ctx, uint32_t us);
} BauraHwAccess;

/* Opérations d'un pilote chargé. */
typedef struct BauraDriverOps {
    int (*probe)(void *drv, const BauraHwAccess *hw);  /* 0 = prend l'appareil */
    int (*start)(void *drv);
    void (*stop)(void *drv);                           /* libère tout */
} BauraDriverOps;

typedef enum {
    BAURA_BIND_NONE = 0, BAURA_BIND_CANDIDATE, BAURA_BIND_PROBING,
    BAURA_BIND_BOUND, BAURA_BIND_FAILED, BAURA_BIND_QUARANTINED
} BauraBindState;

/* Entrée de table de correspondance, du plus précis au plus général. */
typedef struct {
    uint16_t vendor, device;       /* 0xFFFF = joker */
    uint16_t sub_vendor, sub_dev;  /* 0xFFFF = joker */
    uint8_t cls, subcls, prog_if;  /* 0xFF = joker */
    uint8_t rev_min, rev_max;
} BauraMatchEntry;

#define BAURA_ANY16 0xFFFFu
#define BAURA_ANY8 0xFFu

/* Points de précision : plus la valeur est haute, plus la règle est précise. */
static inline int baura_match_score(const BauraMatchEntry *m, uint16_t ven,
    uint16_t dev, uint16_t sven, uint16_t sdev, uint8_t cls, uint8_t sub,
    uint8_t pi, uint8_t rev)
{
    int s = 0;
    if (rev < m->rev_min || rev > m->rev_max) return -1;
    if (m->vendor != BAURA_ANY16) { if (m->vendor != ven) return -1; s += 8; }
    if (m->device != BAURA_ANY16) { if (m->device != dev) return -1; s += 16; }
    if (m->sub_vendor != BAURA_ANY16) { if (m->sub_vendor != sven) return -1; s += 4; }
    if (m->sub_dev != BAURA_ANY16) { if (m->sub_dev != sdev) return -1; s += 4; }
    if (m->cls != BAURA_ANY8) { if (m->cls != cls) return -1; s += 1; }
    if (m->subcls != BAURA_ANY8) { if (m->subcls != sub) return -1; s += 1; }
    if (m->prog_if != BAURA_ANY8) { if (m->prog_if != pi) return -1; s += 1; }
    return s;
}
#endif
