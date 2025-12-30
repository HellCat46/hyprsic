#include "frame_buffer.hpp"
#include "wayland-client-core.h"
#include "wayland-client-protocol.h"
#include "xdg-shell-client-protocol.h"

struct WaylandState {
  // Globals
  struct wl_display *dp;
  struct wl_registry *reg;
  struct wl_compositor *compos;
  struct xdg_wm_base *xdgWmBase;

  // Objects
  struct wl_seat *wlSeat;
  struct wl_surface *surface;
  struct xdg_surface *xdgSurface;
  struct xdg_toplevel *xdgTopLevel;
  
  // Inputs
  struct wl_keyboard *wlKeyboard;
  struct wl_pointer *wlPointer;
  
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
