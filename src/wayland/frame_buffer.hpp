#include <wayland-client-protocol.h>
class WaylandFrameBuffer {
    int fd, width, height, stride, size;
    struct wl_shm_pool *shm_pool;
    struct wl_buffer *buff;
    
    public:
    bool err = 0;
    struct wl_shm *shm;
    
    
    WaylandFrameBuffer(int width, int height);
    wl_buffer* DrawFrame();
};