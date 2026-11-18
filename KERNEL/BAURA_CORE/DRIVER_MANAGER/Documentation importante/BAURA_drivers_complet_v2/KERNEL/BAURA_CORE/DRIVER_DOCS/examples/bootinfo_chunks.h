/* Exemple (proposition) : lecture des chunks "extra" du BootInfo seL4 x86_64.
 * Portable : travaille sur un tampon d'octets, sans en-tête seL4, donc testable
 * sur hôte. Voir 02_BOOTINFO_ET_CONSOLE_FRAMEBUFFER.md. */
#ifndef BAURA_BOOTINFO_CHUNKS_H
#define BAURA_BOOTINFO_CHUNKS_H
#include <stddef.h>
#include <stdint.h>

#define BAURA_BI_ID_PADDING 0u
#define BAURA_BI_ID_X86_VBE 1u
#define BAURA_BI_ID_X86_MBMMAP 2u
#define BAURA_BI_ID_X86_ACPI_RSDP 3u
#define BAURA_BI_ID_X86_FRAMEBUFFER 4u
#define BAURA_BI_ID_X86_TSC_FREQ 5u
#define BAURA_BI_ID_FDT 6u

/* Miroir de multiboot2_fb_t du noyau (défini dans un en-tête interne du noyau,
 * non exposé à l'espace utilisateur). Disposition packed : 22 octets. */
typedef struct __attribute__((packed)) {
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t type; /* 0 indexé, 1 RGB direct, 2 texte EGA (à confirmer) */
} BauraBootFb;

typedef struct {
    uint64_t id;
    uint64_t len; /* inclut l'en-tête de 16 octets */
    const uint8_t *payload;
    size_t payload_len;
} BauraBootChunk;

/* Itère sur les chunks. *offset commence à 0. Retourne 1 si un chunk est lu,
 * 0 à la fin ou si le tampon est incohérent (len < 16, dépassement). Les
 * en-têtes peuvent être non alignés : lecture par memcpy. */
int baura_bootinfo_next_chunk(const uint8_t *extra, size_t extra_len,
                              size_t *offset, BauraBootChunk *out);

/* Cherche le premier chunk d'identifiant `id`. Retourne 1 si trouvé. */
int baura_bootinfo_find_chunk(const uint8_t *extra, size_t extra_len,
                              uint64_t id, BauraBootChunk *out);

/* Lit le chunk framebuffer. Retourne 0 si absent ou trop court. */
int baura_bootinfo_get_fb(const uint8_t *extra, size_t extra_len,
                          BauraBootFb *fb);

#endif
