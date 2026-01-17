#pragma once
#include "../logging/manager.hpp"
//#include "../modules/media/playing_now.hpp"
#include "../modules/sysinfo/battery.hpp"
#include "../modules/sysinfo/disk.hpp"
#include "../modules/sysinfo/memory.hpp"
#include "../modules/sysinfo/network.hpp"
#include "../modules/sysinfo/stats.hpp"
#include "../modules/sysinfo/sys_load.hpp"
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
