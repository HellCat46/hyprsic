#pragma once
#include "../../../app/context.hpp"
#include "fstream"
#include <memory>
#include <vector>

struct BatteryStats {
  short percent;
  float timeTillEmpty;
  float timeTillFull;
};

class Battery {
  std::ifstream capacity, chargeFull, chargeNow, currentNow;
  std::string name;
  AppContext *ctx;

public:
  bool failed;
  Battery(std::string basePath, AppContext *ctx);
  BatteryStats getBatteryStats();
  ~Battery();
};

class Charger {
  std::ifstream status;
  AppContext *ctx;

public:
  bool failed;
  Charger(AppContext *ctx);
  bool isCharging();
  ~Charger();
};

class BatteryInfo {
  std::vector<std::unique_ptr<Battery>> batteries;
  Charger charger;
  int battCount;
  AppContext *ctx;
  bool charging;
  bool lowBattery;

public:
  BatteryInfo(AppContext *ctx);
  BatteryStats getBatteryStats();
  int getBatteryCount();
  
  bool isCharging();
};
