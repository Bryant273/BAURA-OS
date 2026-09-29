
#include <stdint.h>

typedef struct {
    uint32_t ip;
    uint8_t mac[6];
    uint64_t timestamp;
} ARP_Entry;

ARP_Entry arp_cache[1024];

// API 190: BAURA_NET_RESOLVE_MAC
uint8_t* api_net_resolve_mac(uint32_t ip) {
    // 1. Check ARP Cache (L1 Lookup)
    for (int i = 0; i < 1024; i++) {
        if (arp_cache[i].ip == ip && (current_time() - arp_cache[i].timestamp < 300)) {
            return arp_cache[i].mac;
        }
    }

    // 2. Broadcast ARP Request to LAN
    uint8_t request_pkt[42] = { /* ARP Request Header */ };
    net_hw_broadcast(request_pkt);

    // 3. Block until ARP Reply is received
    uint8_t reply_mac[6];
    if (net_wait_for_arp_reply(ip, reply_//mac)) {
        // Store in cache for next time
        add_to_arp_cache(ip, reply_mac);
        return reply_mac;
    }

    return NULL;
}
