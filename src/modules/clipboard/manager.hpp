#pragma once

#include <wayland-client-protocol.h>

class ClipboardManager {
    wl_display *dp;
    wl_registry *registry;
    
    
    static void registry_handler(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
    static void registry_remover(void *data, wl_registry *registry, uint32_t name);
    static constexpr wl_registry_listener registry_listener = {
        .global = registry_handler,
        .global_remove = registry_remover
    };
    
    
    public:
        ClipboardManager();
        ~ClipboardManager();

};