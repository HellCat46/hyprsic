#include "console_preview.hpp"
#include "math.h"

#define TAG "Display"

Display::Display() {
  // Initiating The Network and It's Stats
  net.Init(&logger);
  stat.Init(&logger);
  stat.rx = net.GetTotRx();
  stat.tx = net.GetTotTx();
  stat.time = std::chrono::steady_clock::now();

  load.Init(&logger);
  mem.Init(&logger);

  disk.Init("/home/hellcat", &logger);

  battery.Init(&logger);

  //playing.Init(&logger);

  hyprWS.Init(&logger);
}
std::string Display::DisplayBar() {
  std::string str;

  auto curTime = std::chrono::steady_clock::now();
  auto sec =
      std::chrono::duration_cast<std::chrono::seconds>(curTime - stat.time)
          .count();

  double trx = net.GetTotRx(), ttx = net.GetTotTx();
  str += Stats::ParseBytes(trx, 2) + " (" + Stats::ParseBytes((trx - stat.rx) / sec, 1) +
         ")\t" + Stats::ParseBytes(ttx, 2) + " (" +
         Stats::ParseBytes((ttx - stat.tx) / sec, 1) + ")\t";
  str += std::to_string(load.GetLoad(1)) + " " +
         std::to_string(load.GetLoad(5)) + " " +
         std::to_string(load.GetLoad(15)) + "\t";
  str += Stats::ParseBytes(mem.GetUsedRAM() * 1024, 2) + "/" +
         Stats::ParseBytes(mem.GetTotRAM() * 1024, 2) + " " +
         Stats::ParseBytes(mem.GetUsedSwap() * 1024, 2) + "/" +
         Stats::ParseBytes(mem.GetTotSwap() * 1024, 2) + "\t";

  if (disk.GetDiskInfo(stat.diskAvail, stat.diskTotal) == 0) {
    str += Stats::ParseBytes(stat.diskTotal - stat.diskAvail, 2) + "/" +
           Stats::ParseBytes(stat.diskTotal, 2) + "\t";
  }

  str += std::to_string(battery.getTotPercent()) +
         "\t";

  if (hyprWS.activeWorkspaceId == 0) {
    auto wsEntry = hyprWS.workspaces.find(hyprWS.activeWorkspaceId);
    str += "Active WS: " + wsEntry->second.name + "\t";
  }

  // if (playing.data.out.length() != 0) {
  //   str += playing.data.out + " (" + playing.data.client + ") " + "\t";
  // }

  str += "\n";
  stat.rx = trx;
  stat.tx = ttx;
  stat.time = curTime;
  return str;
}