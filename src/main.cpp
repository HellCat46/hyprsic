#include<iostream>
#include<cstdlib>
#include<wayland-client.h>
#include "Collectors/Network.cpp"
#include "chrono"
#include "Display.hpp"
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
    double rx = net.GetTotRx(), tx = net.GetTotTx();
    while(true){
        double trx = net.GetTotRx(), ttx = net.GetTotTx();
        std::cout<<trx<<"("<<DisplayBytes((trx-rx)/5)<<")"<<"\t"<<ttx<<"("<<DisplayBytes((ttx-tx)/5)<<")"<<std::endl;
        std::this_thread::sleep_for (std::chrono::seconds(5));
        rx = trx;
        tx = ttx;
    }

    return 0;
}

