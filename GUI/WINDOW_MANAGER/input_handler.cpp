void handle_global_shortcuts(uint32_t key_mask) {
    if (key_mask & 0x1 && key_mask & 0x2 && key_mask & 0x4) {
        trigger_security_overlay();
    }
}
void trigger_security_overlay() { /* Blur screen -> Show Lock/SignOut/TaskMgr */ }