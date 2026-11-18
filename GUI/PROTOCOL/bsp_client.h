/**
 * BAURA OS — BSP (Baura Surface Protocol)
 * bsp_client.h — Interface cliente pour connecter une application BUI au serveur d'affichage (BDS)
 */

#ifndef BAURA_BSP_CLIENT_H
#define BAURA_BSP_CLIENT_H

#include "bsp_protocol.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bsp_client_conn bsp_client_conn_t;

/* Connexion au serveur d'affichage BDS */
bsp_client_conn_t* bsp_client_connect(const char* endpoint_name);
void               bsp_client_disconnect(bsp_client_conn_t* conn);

/* Gestion des surfaces clientes */
uint32_t bsp_surface_create(bsp_client_conn_t* conn, bsp_surface_role_t role, int32_t w_dp, int32_t h_dp, const char* title);
void     bsp_surface_destroy(bsp_client_conn_t* conn, uint32_t surface_id);

/* Attachement d'un tampon et validation par zone de dommage (Principe 4) */
bool     bsp_surface_attach_buffer(bsp_client_conn_t* conn, uint32_t surface_id, uint64_t buffer_handle, uint32_t stride, uint32_t format);
bool     bsp_surface_damage_rect(bsp_client_conn_t* conn, uint32_t surface_id, bsp_rect_t rect);
bool     bsp_surface_commit(bsp_client_conn_t* conn, uint32_t surface_id);

/* Réception des événements (souris, clavier, redimensionnement) */
bool     bsp_client_poll_event(bsp_client_conn_t* conn, bsp_msg_input_t* out_evt);

#ifdef __cplusplus
}
#endif

#endif /* BAURA_BSP_CLIENT_H */
