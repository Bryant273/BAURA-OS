/**
 * BAURA OS — BSP SDK
 * bsp_client.c — Implémentation du client BSP pour transport hôte et seL4
 */

#include "bsp_client.h"
#include <stdlib.h>
#include <string.h>

struct bsp_client_conn {
    bool        connected;
    uint32_t    next_surface_id;
    char        endpoint[64];
};

bsp_client_conn_t* bsp_client_connect(const char* endpoint_name) {
    bsp_client_conn_t* conn = (bsp_client_conn_t*)calloc(1, sizeof(bsp_client_conn_t));
    if (!conn) return NULL;

    conn->connected = true;
    conn->next_surface_id = 1;
    if (endpoint_name) {
        strncpy(conn->endpoint, endpoint_name, sizeof(conn->endpoint) - 1);
    }
    return conn;
}

void bsp_client_disconnect(bsp_client_conn_t* conn) {
    if (!conn) return;
    conn->connected = false;
    free(conn);
}

uint32_t bsp_surface_create(bsp_client_conn_t* conn, bsp_surface_role_t role, int32_t w_dp, int32_t h_dp, const char* title) {
    if (!conn || !conn->connected) return 0;
    (void)role; (void)w_dp; (void)h_dp; (void)title;
    return conn->next_surface_id++;
}

void bsp_surface_destroy(bsp_client_conn_t* conn, uint32_t surface_id) {
    if (!conn || !conn->connected) return;
    (void)surface_id;
}

bool bsp_surface_attach_buffer(bsp_client_conn_t* conn, uint32_t surface_id, uint64_t buffer_handle, uint32_t stride, uint32_t format) {
    if (!conn || !conn->connected || !surface_id) return false;
    (void)buffer_handle; (void)stride; (void)format;
    return true;
}

bool bsp_surface_damage_rect(bsp_client_conn_t* conn, uint32_t surface_id, bsp_rect_t rect) {
    if (!conn || !conn->connected || !surface_id) return false;
    (void)rect;
    return true;
}

bool bsp_surface_commit(bsp_client_conn_t* conn, uint32_t surface_id) {
    if (!conn || !conn->connected || !surface_id) return false;
    return true;
}

bool bsp_client_poll_event(bsp_client_conn_t* conn, bsp_msg_input_t* out_evt) {
    if (!conn || !conn->connected || !out_evt) return false;
    return false;
}
