#pragma once
#include "services/header/context.hpp"
#include "fstream"
#include <memory>
#include <vector>

struct BatteryStats {
  short percent;
  float timeTillEmpty;
  float timeTillFull;
};

class Battery {
  AppContext *ctx;
  std::ifstream capacity, chargeFull, chargeNow, currentNow;
  std::string name;

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
  AppContext *ctx;
  std::vector<std::unique_ptr<Battery>> batteries;
  Charger charger;
  int battCount;
  bool charging;
  bool lowBattery;

public:
  BatteryInfo(AppContext *ctx);
  BatteryStats getBatteryStats();
  int getBatteryCount();

  bool isCharging();
};
