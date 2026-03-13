#pragma once

#include "services/header/context.hpp"
#include "gtk/gtk.h"
#include "manager/battery.hpp"
#include "manager/memory.hpp"
#include "manager/stats.hpp"
#include "manager/sys_load.hpp"
#include "manager/temperature.hpp"

class SysInfoModule {
    AppContext *ctx;
    GtkWidget *netWid;
    GtkWidget *tempWid;
    GtkWidget *diskWid;
    GtkWidget *loadWid;
    GtkWidget *memWid;
    GtkWidget *batteryWid;
    GtkWidget *timeWid;
    
    Stats* stat;
    Memory* mem;
    SysLoad* load;
    BatteryInfo* battery;
    TemperatureManager* tempManager;
    
    public:
    SysInfoModule(AppContext*ctx, Stats* stats, Memory* memory, SysLoad* sysLoad, BatteryInfo* batteryInfo, TemperatureManager* tempMgr);
    
    std::vector<GtkWidget *> setup();
    void update();
};