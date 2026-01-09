#pragma once
#include "../../logging/manager.hpp"
#include "fstream"

class Battery {
  std::ifstream capacity;
  std::ifstream status;
  std::string name;
  LoggingManager *logger;

public:
  bool failed;
  int Init(std::string basePath, LoggingManager *logMgr);
  short getBatteryPercent();
  std::string getStatus();
  ~Battery();
};

class BatteryInfo {
  Battery *batteries;
  int battCount;
  LoggingManager *logger;

public:
  int Init(LoggingManager *logMgr);
  short getTotPercent();
  int getBatteryCount();
  ~BatteryInfo();
};
