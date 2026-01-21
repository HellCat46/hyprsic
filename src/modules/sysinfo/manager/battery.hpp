#pragma once
#include "../../../logging/manager.hpp"
#include "fstream"
#include <memory>
#include <vector>

class Battery {
  std::ifstream capacity;
  std::ifstream status;
  std::string name;
  LoggingManager *logger;

public:
  bool failed;
  Battery(std::string basePath, LoggingManager *logMgr);
  short getBatteryPercent();
  std::string getStatus();
  ~Battery();
};

class BatteryInfo {
  std::vector<std::unique_ptr<Battery>> batteries;
  int battCount;
  LoggingManager *logger;

public:
  BatteryInfo(LoggingManager *logMgr);
  short getTotPercent();
  int getBatteryCount();
};
