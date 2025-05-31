#include "listener.hpp"
#include "manager.hpp"
#include <cstring>
#include <iostream>
#include <ostream>
#include <wayland-client-protocol.h>

void WaylandListener::registryHandleGlobal(void *data, struct wl_registry *reg,
                                           uint32_t name, const char *interface,
                                           uint32_t version) {
  class WaylandState *state = static_cast<WaylandState *>(data);
  if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
    state->compos = static_cast<struct wl_compositor *>(
        wl_registry_bind(reg, name, &wl_compositor_interface, version));
        std::cout<<"[INFO] Successfully Initiated Compositer for Wayland."<<std::endl;
  }else if(std::strcmp(interface, wl_shm_interface.name) == 0){
      state->shm = static_cast<struct wl_shm *>(wl_registry_bind(reg, name, &wl_shm_interface, version));
      std::cout<<"[INFO] Successfully Initiated Shared Memory for Wayland."<<std::endl;
  }
}

void WaylandListener::registryHandleGlobalRemove(void *data,
                                                 struct wl_registry *reg,
                                                 uint32_t name) {
  std::cout << "Name: " << name << std::endl;
}

void WaylandListener::bufferRelease(void* data, struct wl_buffer *buff){
    wl_buffer_destroy(buff);
} 