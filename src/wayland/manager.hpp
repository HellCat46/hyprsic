#include "frame_buffer.hpp"
#include "wayland-client-core.h"
#include "wayland-client-protocol.h"
#include "xdg-shell-client-protocol.h"

struct WaylandState {
  // Globals
  struct wl_display *dp;
  struct wl_registry *reg;
  struct wl_compositor *compos;
  struct xdg_wm_base *xdg_wm_base;

  // Objects
  struct wl_surface *surface;
  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *xdg_toplevel;
  
  // Buffer and Shared Memory
  WaylandFrameBuffer* frameBuffer;
};


class WaylandManager {
  struct WaylandState state;
  
  public:
  WaylandManager();
  int printInfo();
  ~WaylandManager();
};
