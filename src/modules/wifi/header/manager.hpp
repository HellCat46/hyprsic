#pragma once

#include "services/header/context.hpp"
#include "dbus/dbus.h"
#include <string>
#include <unordered_map>

struct WifiStation {
  std::string ssid, type;
  bool known, autoConn;
  short rssi;
};

class WifiManager {
  AppContext *ctx;
  std::string devPath, devAddr, devName, devAdapter, agentPath;
  bool powered = false, scanning = false;
  std::string connDev, authDev;
  DBusMessage* authMsg;
  
  void RegisterAgent(bool reg);
  void GetManagedObjects();
  

  int GetConnectedDevice();
  void GetDevices();
  int GetDeviceInfo(std::string devPath, WifiStation &station);
  
  // Action methods
  void Scan();
  void Connect(const std::string &networkPath);
  void Disconnect();
  void Forget(const std::string &networkPath);
  void SubmitPassphrase(const std::string& password);

public:
  std::unordered_map<std::string, WifiStation> devices;
  
  WifiManager(AppContext *appCtx);
  ~WifiManager();
  void update(bool force = false);

  bool IsPowered() const;
  bool IsScanning() const;
  
  // Monitor Changes Functions
  void addMatchRulesDbus();
  void handleRequestPassphraseDbus(DBusMessage* msg, DBusMessageIter &rootIter);
  void handleRequestCancelDbus();
  void handleInterfacesRemovedDbus(DBusMessageIter &rootIter);
  void handlePropertiesChangedDbus(DBusMessage* msg, DBusMessageIter &rootIter);


};
