#include "manager.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <ostream>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

WaylandManager::WaylandManager() {
  state.dp = wl_display_connect(NULL);
  state.compos = 0;

  if (!state.dp) {
    std::cerr << "[ERROR] Failed to connect to Wayland Display" << std::endl;
    return;
  }

  std::cout << "[INFO] Wayland Display Connection Established" << std::endl;
  state.reg = wl_display_get_registry(state.dp);

  wl_registry_add_listener(state.reg, &regListener, &state);
  wl_display_roundtrip(state.dp);
  
  state.surface = wl_compositor_create_surface(state.compos);
  
  
  allocateSharedMemory(1920,1080);
}

int WaylandManager::allocateSharedMemory(int width, int height){
    int stride = width*4;
    int size = height * stride * 2;
    
    shMem.fd = memfd_create("hyprsicSHM", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if(shMem.fd < 0){
        std::cerr<<"[ERROR] Failed to create the File Descriptor for Shared Memory."<<std::endl;
        return -1;
    }
    
    try {
        int ret;
        do {
            ret = ftruncate(shMem.fd, size);
        }while(ret < 0);
    
    }catch(...){
        std::cerr<<"[ERROR] Failed to Truncate the File Descriptor for Shared Memory."<<std::endl;
        return -1;
    }
    
    uint8_t *poolData = static_cast<uint8_t*>(mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shMem.fd, 0));
    shMem.shm_pool = wl_shm_create_pool(state.shm, shMem.fd, size);
    
    shMem.buff = wl_shm_pool_create_buffer(shMem.shm_pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
    
    uint32_t *pixels = reinterpret_cast<uint32_t*>(&poolData[0]);    
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        if ((x + y / 8 * 8) % 16 < 8) {
          pixels[y * width + x] = 0xFF666666;
        } else {
          pixels[y * width + x] = 0xFFEEEEEE;
        }
      }
    }

    return 0;
}


WaylandManager::~WaylandManager() {
  if (state.dp) {
    wl_display_disconnect(state.dp);
  }
  
  if(shMem.fd >= 0){
      close(shMem.fd);
  }
}
