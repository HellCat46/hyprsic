#include "battery.hpp"
#include "../../../utils/helper_func.hpp"
#include "cstring"
#include "dirent.h"
#include "fstream"
#include "vector"
#include <string>

#define TAG "Battery"

Battery::Battery(std::string basePath, LoggingManager *logMgr) {
  logger = logMgr;

  std::string msg = "Initiating Info Object for Battery ";
  msg += basePath.substr(basePath.rfind("/") + 1, basePath.size());
  msg += ".";
  logger->LogInfo(TAG, msg);

  capacity.open(basePath + "/capacity", std::ios::in);
  status.open(basePath + "/status", std::ios::in);

  if (!capacity.is_open() || !status.is_open()) {
    logger->LogError(TAG, "Failed to Open Files Required for Battery Stats");

    failed = 1;
  }

  std::ifstream manufacturer, model;
  manufacturer.open(basePath + "/manufacturer", std::ios::in);
  model.open(basePath + "/model_name", std::ios::in);

  if (!manufacturer.is_open() || !model.is_open()) {
    logger->LogError(TAG, "Failed to Get Battery Manufacturer or Model Name");
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

short Battery::getBatteryPercent() {
  short percent = 0;
  char ch;

  while (capacity.get(ch)) {
    if (ch >= 48 && ch <= 57)
      percent = (percent * 10) + (ch - 48);
  }
  capacity.clear();
  capacity.seekg(0);

  return percent;
}

std::string Battery::getStatus() {
  char in[100];

  status.getline(in, 100);
  status.clear();
  status.seekg(0);

  return in;
}

Battery::~Battery() {
  capacity.close();
  status.close();
}

BatteryInfo::BatteryInfo(LoggingManager *logMgr) : logger(logMgr) {

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
      batteries.push_back(std::make_unique<Battery>(basePath + ent->d_name, logger));
    }
  }

  battCount = battPaths.size();
}

short BatteryInfo::getTotPercent() {
  float avg = 0;
  char ch;

  for (int idx = 0; idx < battCount; idx++) {
    if (batteries[idx]->failed)
      continue;
    avg = ((avg * (idx)) + batteries[idx]->getBatteryPercent()) / (idx + 1);
  }

  return avg;
}

int BatteryInfo::getBatteryCount() { return battCount; }
