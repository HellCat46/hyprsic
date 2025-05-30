
#include "../collectors/battery.hpp"
#include "../collectors/disk.hpp"
#include "../collectors/memory.hpp"
#include "../collectors/network.hpp"
#include "../collectors/playing_now.hpp"
#include "../collectors/sys_load.hpp"
#include "../collectors/workspaces/hyprland.hpp"
#include "chrono"
#include "math.h"

struct Stats {
  double rx, tx;
  unsigned long diskAvail, diskTotal;
  std::chrono::time_point<std::chrono::steady_clock> time;
};

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
  std::string DisplayBytes(double bytes, int precision);
};
