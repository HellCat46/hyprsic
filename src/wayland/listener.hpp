#include "cstdint"
#include "wayland-client-protocol.h"
namespace WaylandListener {
    // Global Registry Listeners
    void registryHandleGlobal(void *data, struct wl_registry *reg, uint32_t name,
                              const char *interface, uint32_t version);
    void registryHandleGlobalRemove(void *data, struct wl_registry *reg,
                                    uint32_t name);
    
    // Buffer Listeners
    void bufferRelease(void* data, struct wl_buffer *buff);
}