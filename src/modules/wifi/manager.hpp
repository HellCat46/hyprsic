#pragma once

#include "../../services/header/context.hpp"
#include "dbus/dbus.h"
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
  bool powered = false, scanning = false;
  std::unordered_map<std::string, WifiStation> devices;

  int GetConnectedDevice();
  void GetDevices();
  int GetDeviceInfo(std::string devPath, WifiStation &station);

public:
  WifiManager(AppContext *appCtx);
  void update();

  bool IsPowered() const;
  bool IsScanning() const;
  WifiStation ConnectedDevice();
  
  // Monitor Changes Functions
  void addMatchRules();
  void handlePropertiesChanged(DBusMessage* msg, DBusMessageIter &rootIter);

  // Action methods
  void Scan();
  int Connect(const std::string &networkPath);
  void Disconnect();
  void Forget(const std::string &networkPath);
};
