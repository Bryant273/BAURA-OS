#include "BAURA_MESSAGE_BUS.h"

void bus_publish(uint32_t topic, void* data, uint32_t size) {
    // Le bus utilise l'IPC L4 pour diffuser le message aux abonnés
    // Implémentation simplifiée : on envoie au "Bus Manager" qui redistribue
    L4_IPC_Message msg;
    msg.label = topic;
    msg.shared_buffer = data;
    msg.buffer_size = size;

    l4_ipc_send(CAP_MESSAGE_BUS, &msg);
}

void bus_subscribe(uint32_t topic, void (*callback)(BusMessage*)) {
    // Enregistre le callback auprès du Bus Manager
    L4_IPC_Message msg;
    msg.label = TOPIC_SUBSCRIBE;
    msg.payload[0] = topic;
    msg.payload[1] = (uintptr_t)callback;

    l4_ipc_send(CAP_MESSAGE_BUS, &msg);
}