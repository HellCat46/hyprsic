#include<iostream>
#include<cstdlib>
#include<wayland-client.h>

int main(int argc, const char * argv[])
{
    struct wl_display *display = wl_display_connect(NULL);
    if (!display)
    {
        std::cerr << "wl_display_connect failed" << std::endl;
        exit(1);
        return 1;
    }

    std::cout << "wl_display_connect success" << std::endl;

    wl_display_disconnect(display);
    return 0;
}