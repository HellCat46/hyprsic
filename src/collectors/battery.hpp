#pragma once
#include "fstream"

class Battery {
  std::ifstream capacity;
  std::ifstream status;
  std::string name;
  public:
  	bool failed;
    int Init(std::string basePath);
    short getBatteryPercent();
    std::string getStatus();
    ~Battery();
};

class BatteryInfo {
  Battery* batteries;
  int battCount;
  public:
    int Init();
    short getTotPercent();
    int getBatteryCount();
    ~BatteryInfo();
};