#include "iostream"
#include "cstdlib"
#include "Managers/Bluetooth.cpp"
//#include "Hyprland/Workspaces.cpp"
//#include "Collectors/PlayingNow.cpp"
// #include "Collectors/Bluetooth.cpp"
#include "wayland-client.h"
#include "chrono"
#include "Display.cpp"
#include "thread"
#include "cstring"

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

    DbusSystem dbusInstance;





    BluetoothManager bm(&dbusInstance);
    bm.switchDiscovery();
std::cout<<"Bluetooth Manager Initialized"<<std::endl;
//  BluetoothDevice bl(&dbusInstance);
//    PlayingNow pn;
//    pn.Init();

    //Display dp;
    while(true){
        bm.getDeviceList();
        //bl.getDeviceList();
        //bl.printDevicesInfo();
        //std::cout<<dp.DisplayBar();
        std::this_thread::sleep_for (std::chrono::seconds(5));
    }

    return 0;
}

