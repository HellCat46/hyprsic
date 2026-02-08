#include "battery.hpp"
#include "../../../utils/helper_func.hpp"
#include "cstring"
#include "dirent.h"
#include "fstream"
#include "vector"
#include <filesystem>
#include <string>

#define TAG "Battery"

Battery::Battery(std::string basePath, AppContext *ctx) : ctx(ctx) {
  std::string msg = "Initiating Info Object for Battery ";
  msg += basePath.substr(basePath.rfind("/") + 1, basePath.size());
  msg += ".";
  ctx->logger.LogInfo(TAG, msg);

  capacity.open(basePath + "/capacity", std::ios::in);
  chargeFull.open(basePath + "/charge_full", std::ios::in);
  chargeNow.open(basePath + "/charge_now", std::ios::in);
  currentNow.open(basePath + "/current_now", std::ios::in);

  if (!capacity.is_open()) {
    ctx->logger.LogError(TAG,
                         "Failed to Open Files Required for Battery Stats");

    failed = 1;
  }

  std::ifstream manufacturer, model;
  manufacturer.open(basePath + "/manufacturer", std::ios::in);
  model.open(basePath + "/model_name", std::ios::in);

  if (!manufacturer.is_open() || !model.is_open()) {
    ctx->logger.LogError(TAG,
                         "Failed to Get Battery Manufacturer or Model Name");
    name = "Unknown";
  } else {
    char in[30];
    model.getline(in, 30);
    name += in;
    manufacturer.getline(in, 30);
    name += " (";
    name += in;
    name += ')';

    model.close();
    manufacturer.close();
  }

  failed = 0;
}

BatteryStats Battery::getBatteryStats() {
  BatteryStats stats;
  char in[100];

  // Get Battery Percent
  capacity.getline(in, 100);
  stats.percent = atoi(in);
  capacity.clear();
  capacity.seekg(0);

  // Read Charge Full, Charge Now and Current Now
  chargeFull.getline(in, 100);
  float full = atoi(in);
  chargeFull.clear();
  chargeFull.seekg(0);

  chargeNow.getline(in, 100);
  float now = atoi(in);
  chargeNow.clear();
  chargeNow.seekg(0);

  currentNow.getline(in, 100);
  float current = atoi(in);
  currentNow.clear();
  currentNow.seekg(0);

  if (current == 0) {
    stats.timeTillEmpty = -1;
    stats.timeTillFull = -1;
    return stats;
  }

  // Calculate Time Till Empty and Time Till Full
  stats.timeTillEmpty = now / current;
  stats.timeTillFull = (full - now) / current;

  return stats;
}

Battery::~Battery() {
  capacity.close();
  chargeFull.close();
  chargeNow.close();
  currentNow.close();
}

Charger::Charger(AppContext *ctx) : ctx(ctx) {
  for (auto &entry :
       std::filesystem::directory_iterator("/sys/class/power_supply/")) {
    if (!entry.is_directory() ||
        entry.path().filename().string().find("source") != std::string::npos)
      continue;

    bool isCharger = false;
    for (auto &inEn : std::filesystem::directory_iterator(entry.path())) {
      if (inEn.is_regular_file() && inEn.path().filename() == "online") {
        isCharger = true;
        break;
      }
    }

    if (isCharger) {
      status.open(entry.path().string() + "/online", std::ios::in);
      ctx->logger.LogInfo(TAG, "Charger Found:" + entry.path().string());
      break;
    }
  }

  if (status.is_open()) {
    failed = 0;
  } else {
    ctx->logger.LogError(TAG, "Failed to Open Charger Status File");
    failed = 1;
  }
}

bool Charger::isCharging() {
  if (failed)
    return false;

  char ch;
  status.read(&ch, 1);
  status.clear();
  status.seekg(0);

  return ch == '1';
}

Charger::~Charger() { status.close(); }

BatteryInfo::BatteryInfo(AppContext *ctx) : ctx(ctx), charger(ctx) {

  DIR *dir;
  std::string basePath = "/sys/class/power_supply/";
  char folderStr[] = "BAT";

  dir = opendir(basePath.c_str());

  if (dir == nullptr) {
    return;
  }

  std::vector<std::string> battPaths;
  for (dirent *ent = readdir(dir); ent != nullptr; ent = readdir(dir)) {
    if (HelperFunc::saferStrNCmp(folderStr, ent->d_name, 3)) {
      battPaths.push_back(ent->d_name);
      batteries.push_back(
          std::make_unique<Battery>(basePath + ent->d_name, ctx));
    }
  }

  battCount = battPaths.size();
  charging = charger.isCharging();
}

BatteryStats BatteryInfo::getBatteryStats() {
  BatteryStats avgStats{0, 0, 0};
  char ch;

  for (int idx = 0; idx < battCount; idx++) {
    if (batteries[idx]->failed)
      continue;
    BatteryStats stats = batteries[idx]->getBatteryStats();

    avgStats.percent += stats.percent;
    avgStats.timeTillEmpty += stats.timeTillEmpty;
    avgStats.timeTillFull += stats.timeTillFull;
  }

  // Get Average Stats
  avgStats.percent /= battCount;
  avgStats.timeTillEmpty /= battCount;
  avgStats.timeTillEmpty *= 60;
  avgStats.timeTillFull /= battCount;
  avgStats.timeTillFull *= 60;

  if (avgStats.percent > 20) {
    lowBattery = false;
  } else if (!lowBattery) {
    lowBattery = true;
    ctx->showUpdateWindow(UpdateModule::BATTERY, "battery_low", "Battery Low. " + HelperFunc::convertToTime(avgStats.timeTillEmpty) + " Remaining");
  }

  return avgStats;
}

int BatteryInfo::getBatteryCount() { return battCount; }

bool BatteryInfo::isCharging() {
  bool charging = charger.isCharging();

  if (charging == this->charging) {
    return charging;
  }

  if (charging) {
    ctx->showUpdateWindow(UpdateModule::BATTERY, "charging_on",
                          "Charger Connected");
  } else {
    ctx->showUpdateWindow(UpdateModule::BATTERY, "charging_off",
                          "Charger Disconnected");
  }

  this->charging = charging;
  return charging;
}
