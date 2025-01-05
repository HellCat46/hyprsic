#include "iostream"
#include "cstdlib"
#include "Hyprland/Workspaces.cpp"
#include "wayland-client.h"
#include "chrono"
#include "Display.cpp"
#include "thread"

int main(int argc, const char * argv[])
{
    struct wl_display *display = wl_display_connect(nullptr);
    if (!display)
    {
        std::cerr << "wl_display_connect failed" << std::endl;
        exit(1);
        return 1;
    }

    std::cout << "wl_display_connect success" << std::endl;

    wl_display_disconnect(display);






  //HyprWorkspaces hyprWS;
  //hyprWS.Init();

    Display dp;
    while(true){
        std::cout<<dp.DisplayBar();
        std::this_thread::sleep_for (std::chrono::seconds(5));
    }

    return 0;
}

