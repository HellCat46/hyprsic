#pragma once

#include "gtk/gtk.h"
#include "manager/battery.hpp"
#include "manager/memory.hpp"
#include "manager/stats.hpp"
#include "manager/sys_load.hpp"

class SysInfoModule {
    GtkWidget *netWid;
    GtkWidget *diskWid;
    GtkWidget *loadWid;
    GtkWidget *memWid;
    GtkWidget *batteryWid;
    GtkWidget *timeWid;
    
    Stats* stat;
    Memory* mem;
    SysLoad* load;
    BatteryInfo* battery;
    
    public:
    SysInfoModule(Stats* stats, Memory* memory, SysLoad* sysLoad, BatteryInfo* batteryInfo);
    
    void setup(GtkWidget* gridBox);
    void update();
};