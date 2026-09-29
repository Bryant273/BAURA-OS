#include "BAURA_RPC_CORE.h"

int baura_rpc_call(Capability server_cap, uint32_t method_id, void* args, void* response) {
    L4_IPC_Message msg;
    msg.label = method_id;

    // Copie des arguments dans le payload IPC
    for(int i=0; i<4; i++) {
        msg.payload[i] = ((uint64_t*)args)[i];
    }

    // Envoi via L4 IPC
    if (l4_ipc_send(server_cap, &msg) != SUCCESS) {
        return ERR_IPC_FAILED;
    }

    // Réception de la réponse
    L4_IPC_Message res_msg;
    if (l4_ipc_recv(&res_msg) != SUCCESS) {
        return ERR_IPC_FAILED;
    }

    // Copie du résultat vers le buffer de réponse
    for(int i=0; i<4; i++) {
        ((uint64_t*)response)[i] = res_msg.payload[i];
    }

    return SUCCESS;
}