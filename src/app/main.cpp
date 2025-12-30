#include "iostream"
#include "cstdlib"
#include "chrono"
#include "../debug/console_preview.hpp"
#include "../services/bluetooth.hpp"
#include "thread"
#include "cstring"
#include "window.hpp"

int main(int argc, char ** argv)
{
    MainWindow win;
    win.RunApp();
    
    AppContext ctx;   
    


    Display dp;
    while(true){
        std::cout<<dp.DisplayBar();
        std::this_thread::sleep_for (std::chrono::seconds(5));
    }

    return 0;
}