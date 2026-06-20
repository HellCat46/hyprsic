#pragma once

#include "gtkmm/label.h"
#include "manager/battery.hpp"
#include "manager/memory.hpp"
#include "manager/stats.hpp"
#include "manager/sys_load.hpp"
#include "manager/temperature.hpp"
#include "services/header/context.hpp"

class SysInfoModule {
  AppContext *ctx;
  Gtk::Label netWid;
  Gtk::Label tempWid;
  Gtk::Label diskWid;
  Gtk::Label loadWid;
  Gtk::Label memWid;
  Gtk::Label batteryWid;
  Gtk::Label timeWid;

  Stats *stat;
  Memory *mem;
  SysLoad *load;
  BatteryInfo *battery;
  TemperatureManager *tempManager;

public:
  SysInfoModule(AppContext *ctx, Stats *stats, Memory *memory, SysLoad *sysLoad,
                BatteryInfo *batteryInfo, TemperatureManager *tempMgr);

  void setup(std::vector<std::reference_wrapper<Gtk::Label>>& widgets);
  void update();
};
