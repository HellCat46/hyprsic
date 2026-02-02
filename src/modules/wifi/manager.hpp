#pragma once

#include "../../app/context.hpp"
#include <string>
#include <unordered_map>

struct WifiStation {
  std::string ssid, type;
  bool connected;
  short rssi;
};

class WifiManager {
  AppContext *ctx;
  std::string devPath, devAddr, devName, devAdapter, connectedDev;
  bool powered = false;
  std::unordered_map<std::string, WifiStation> devices;

public:
  WifiManager(AppContext *appCtx);

  int GetConnectedDevice();
  void GetDevices();
  int GetDeviceInfo(std::string devPath, WifiStation &station);
};
