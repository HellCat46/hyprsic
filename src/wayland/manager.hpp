#include "listener.hpp"
#include "wayland-client-core.h"
#include "wayland-client-protocol.h"

struct WaylandState {
  // Globals
  struct wl_display *dp;
  struct wl_registry *reg;
  struct wl_compositor *compos;
  struct wl_shm *shm;
  struct xdg_wm_base *xdg_wm_base;

  // Objects
  struct wl_surface *surface;
  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *xdg_toplevel;
};

struct SharedMem {
  int fd, width, height;
  struct wl_shm_pool *shm_pool;
  struct wl_buffer *buff;
};

class WaylandManager {
  struct WaylandState state;
  struct SharedMem shMem;

  int allocateSharedMemory(int width, int height);
  // Listener Objects
  const struct wl_registry_listener regListener = {
      .global = WaylandListener::registryHandleGlobal,
      .global_remove = WaylandListener::registryHandleGlobalRemove};
  const struct wl_buffer_listener buffListener = {
      .release = WaylandListener::bufferRelease
  };
public:
  WaylandManager();
  int printInfo();
  ~WaylandManager();
};
