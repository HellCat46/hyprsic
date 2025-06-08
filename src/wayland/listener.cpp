#include "listener.hpp"
#include "manager.hpp"
#include "xdg-shell-client-protocol.h"
#include <cstring>
#include <iostream>
#include <ostream>
#include <wayland-client-protocol.h>

void WaylandListener::registryHandleGlobal(void *data, struct wl_registry *reg,
                                           uint32_t name, const char *interface,
                                           uint32_t version) {
  struct WaylandState *state = static_cast<WaylandState *>(data);

  if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
    state->compos = static_cast<struct wl_compositor *>(
        wl_registry_bind(reg, name, &wl_compositor_interface, version));

    std::cout << "[INFO] Successfully Initiated Compositer for Wayland."
              << std::endl;
  } else if (std::strcmp(interface, wl_shm_interface.name) == 0) {
    state->frameBuffer->shm = static_cast<struct wl_shm *>(
        wl_registry_bind(reg, name, &wl_shm_interface, version));

    std::cout << "[INFO] Successfully Initiated Shared Memory for Wayland."
              << std::endl;
  } else if (std::strcmp(interface, xdg_wm_base_interface.name) == 0) {
    
    state->xdg_wm_base = static_cast<struct xdg_wm_base *>(
        wl_registry_bind(reg, name, &xdg_wm_base_interface, version));
    xdg_wm_base_add_listener(state->xdg_wm_base, &xdgWMBaseListener, state);

    std::cout << "[INFO] Successfully Initiated XDG Window Base and Added "
                 "Listener to it"
              << std::endl;
  }
}

void WaylandListener::registryHandleGlobalRemove(void *data,
                                                 struct wl_registry *reg,
                                                 uint32_t name) {
  std::cout << "Name: " << name << std::endl;
}

void WaylandListener::bufferRelease(void *data, struct wl_buffer *buff) {
  wl_buffer_destroy(buff);
}

void WaylandListener::xdgWMBasePing(void *data, struct xdg_wm_base *xdgWMBase,
                                    uint32_t serial) {
  std::cout << "Received a Ping from XDG Window Base." << std::endl;
  xdg_wm_base_pong(xdgWMBase, serial);
}

void WaylandListener::xdgSurfaceConfigure(void *data,
                                          struct xdg_surface *xdgSurface,
                                          uint32_t serial) {

  std::cout << "Configuring XDG Surface..." << std::endl;
  struct WaylandState *state = static_cast<struct WaylandState *>(data);
  xdg_surface_ack_configure(xdgSurface, serial);

  std::cout<<"Drawing Frame into buffer and attaching to Surface"<<std::endl;
  struct wl_buffer *buff = state->frameBuffer->DrawFrame();
  wl_surface_attach(state->surface, buff, 0, 0);
  wl_surface_commit(state->surface);
}
