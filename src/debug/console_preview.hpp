#pragma once
#include "../logging/manager.hpp"
#include "../modules/sysinfo/manager/battery.hpp"
#include "../modules/sysinfo/manager/disk.hpp"
#include "../modules/sysinfo/manager/memory.hpp"
#include "../modules/sysinfo/manager/network.hpp"
#include "../modules/sysinfo/manager/stats.hpp"
#include "../modules/sysinfo/manager/sys_load.hpp"
#include "../modules/workspaces/hyprland/manager.hpp"
#include "math.h"

class Display {
  LoggingManager logger;
  Stats stat;
  Network net;
  SysLoad load;
  Memory mem;
  Disk disk;
  BatteryInfo battery;
 // PlayingNow playing;
  HyprWSManager hyprWS;

public:
  Display();
  std::string DisplayBar();
};
