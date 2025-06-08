#include "frame_buffer.hpp"
#include "listener.hpp"
#include "iostream"
#include "cstdint"
#include "ostream"
#include "sys/mman.h"
#include "unistd.h"
#include <cstddef>
#include <wayland-client-protocol.h>

WaylandFrameBuffer::WaylandFrameBuffer(int width, int height){
    this->width = width;
    this->height = height;
    stride = width*4;
    size = height * stride * 2;
    
}

wl_buffer* WaylandFrameBuffer::DrawFrame(){
    fd = memfd_create("hyprsicSHM", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if(fd < 0){
        std::cerr<<"[ERROR] Failed to create the File Descriptor for Shared Memory."<<std::endl;
        err =1;
        return nullptr;
    }
    
    try {
        int ret;
        do {
            ret = ftruncate(fd, size);
        }while(ret < 0);
    
    }catch(...){
        std::cerr<<"[ERROR] Failed to Truncate the File Descriptor for Shared Memory."<<std::endl;
        err = 1;
        return nullptr;
    }
    uint8_t *poolData = static_cast<uint8_t*>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    
    shm_pool = wl_shm_create_pool(shm, fd, size);
    
    buff = wl_shm_pool_create_buffer(shm_pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(shm_pool);
    close(fd);

    
    uint32_t *pixels = reinterpret_cast<uint32_t*>(&poolData[0]);    
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        if ((x + y / 8 * 8) % 16 < 8) {
          pixels[y * width + x] = 0xFF89B4FA;
        } else {
          pixels[y * width + x] = 0xFF1E1E2E;
        }
      }
    }
    
    munmap(pixels, size);
    wl_buffer_add_listener(buff, &WaylandListener::buffListener, nullptr);
    return buff;
}