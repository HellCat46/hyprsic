#pragma once
#include "../collectors/sysinfo/battery.hpp"
#include "../collectors/sysinfo/disk.hpp"
#include "../collectors/sysinfo/memory.hpp"
#include "../collectors/sysinfo/network.hpp"
#include "../collectors/playing_now.hpp"
#include "../collectors/sysinfo/sys_load.hpp"
#include "../collectors/workspaces/hyprland.hpp"
#include "../collectors/sysinfo/stats.hpp"
#include "math.h"


class Display {
  Stats stat;
  Network net;
  SysLoad load;
  Memory mem;
  Disk disk;
  BatteryInfo battery;
  PlayingNow playing;
  HyprWorkspaces hyprWS;

public:
  Display();
  std::string DisplayBar();
};
