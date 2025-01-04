#include<iostream>
#include<cstdlib>
#include<wayland-client.h>
#include "Collectors/Network.cpp"
#include "chrono"
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





    Network net;
    net.Init();

    std::cout<<"Rx\t\t\t\tTx"<<std::endl;
    while(true){
        std::cout<<std::fixed<<net.GetRx("Gib")<<"\t"<<net.GetTx("Gib")<<std::endl;
        std::this_thread::sleep_for (std::chrono::seconds(5));
    }

    return 0;
}