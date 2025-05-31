#include "iostream"
#include "cstdlib"
#include "chrono"
#include "../debug/console_preview.hpp"
#include "../services/bluetooth.hpp"
#include "thread"
#include "cstring"
#include "../wayland/manager.hpp"

int main(int argc, const char * argv[])
{
    AppContext ctx;
    WaylandManager wm;
    


    Display dp;
    while(true){
        std::cout<<dp.DisplayBar();
        std::this_thread::sleep_for (std::chrono::seconds(5));
    }

    return 0;
}

