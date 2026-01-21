#pragma once

#include "disk.hpp"
#include "network.hpp"
#include "../../../logging/manager.hpp"
#include <chrono>

class Stats {

  Network net;
  Disk disk;
  LoggingManager *logger;

public:
  double rx, tx;
  unsigned long diskAvail, diskTotal;
  std::chrono::time_point<std::chrono::steady_clock> time;

  Stats();
  void Init(LoggingManager *logMgr);
  void UpdateData();
  std::string GetNetRx();
  std::string GetNetTx();
  std::string GetDiskAvail();
  std::string GetDiskTotal();

  static std::string ParseBytes(double bytes, int precision);
};