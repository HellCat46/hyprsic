#pragma once
#include "../modules/sysinfo/header/manager/battery.hpp"
#include "../modules/sysinfo/header/manager/disk.hpp"
#include "../modules/sysinfo/header/manager/memory.hpp"
#include "../modules/sysinfo/header/manager/network.hpp"
#include "../modules/sysinfo/header/manager/stats.hpp"
#include "../modules/sysinfo/header/manager/sys_load.hpp"
#include "../modules/workspaces/hyprland/header/manager.hpp"
#include "math.h"

class Display {
  AppContext ctx;
  HyprWSManager hyprWS;
  Network net;
  Stats stat;
  SysLoad load;
  Memory mem;
  Disk disk;
  BatteryInfo battery;
  // PlayingNow playing;

public:
  Display();
  std::string DisplayBar();
};
