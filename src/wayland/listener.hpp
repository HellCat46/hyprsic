#include "cstdint"
#include "wayland-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include <cstdint>
namespace WaylandListener {
// Global Registry Listeners
void registryHandleGlobal(void *data, struct wl_registry *reg, uint32_t name,
                          const char *interface, uint32_t version);
void registryHandleGlobalRemove(void *data, struct wl_registry *reg,
                                uint32_t name);

// Window Listeners
void xdgWMBasePing(void *data, struct xdg_wm_base *xdgWMBase, uint32_t serial);

// Buffer Listeners
void bufferRelease(void *data, struct wl_buffer *buff);

// XDG Surface Listeners
void xdgSurfaceConfigure(void *data, struct xdg_surface *xdgSurface,
                         uint32_t serial);

// Listener Objects
const struct wl_registry_listener regListener = {
    .global = registryHandleGlobal,
    .global_remove = registryHandleGlobalRemove};

const struct wl_buffer_listener buffListener = {.release = bufferRelease};

const struct xdg_wm_base_listener xdgWMBaseListener = {.ping = xdgWMBasePing};

const struct xdg_surface_listener xdgSurfaceListener = {
    .configure = xdgSurfaceConfigure};

} // namespace WaylandListener
