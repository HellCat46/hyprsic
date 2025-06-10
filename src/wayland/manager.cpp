#include "manager.hpp"
#include "listener.hpp"
#include "xdg-shell-client-protocol.h"
#include <cstring>
#include <iostream>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

WaylandManager::WaylandManager() {
  state.dp = wl_display_connect(nullptr);
  state.compos = 0;
  

  if (!state.dp) {
    std::cerr << "[ERROR] Failed to connect to Wayland Display" << std::endl;
    return;
  }

  std::cout << "[INFO] Wayland Display Connection Established" << std::endl;
  state.reg = wl_display_get_registry(state.dp);
  
  state.frameBuffer = new WaylandFrameBuffer(1920,1080);
  if(state.frameBuffer->err){
      return;
  }

  wl_registry_add_listener(state.reg, &WaylandListener::regListener, &state);
  wl_display_roundtrip(state.dp);
  
  std::cout<<"[INFO] Successfully Initiated all the required instances"<<std::endl;
  
  
  state.surface = wl_compositor_create_surface(state.compos);
  state.xdgSurface = xdg_wm_base_get_xdg_surface(state.xdgWmBase, state.surface);
  xdg_surface_add_listener(state.xdgSurface, &WaylandListener::xdgSurfaceListener, &state);
  std::cout<<"[INFO] Successfully Created Wayland and XDG Surface. Also Added Listener to XDG Surface."<<std::endl;
  
  state.xdgTopLevel = xdg_surface_get_toplevel(state.xdgSurface);
  xdg_toplevel_set_title(state.xdgTopLevel, "Hyprsic");
  wl_surface_commit(state.surface);
  
  while (wl_display_dispatch(state.dp)) {
      
  }
}


WaylandManager::~WaylandManager() {
  if (state.dp) {
    wl_display_disconnect(state.dp);
  }
  
  // Surely... Null is assigned to pointer by default.
  if(state.frameBuffer){
      delete state.frameBuffer;
  }
}
