#pragma once

#include "services/header/context.hpp"
#include "cstring"
#include "dbus/dbus.h"
#include "string"
#include "unordered_map"
#include <string>
#include <string_view>
#include <unordered_map>

struct Device {
  std::string addr, name, path, deviceType;
  short rssi;
  bool paired, trusted, connected, mediaConnected;
  short batteryPer;
};

enum DevicePropFlags {
  NAME = 1,
  ADDRESS = 2,
  CONNECTED = 4,
  PAIRED = 4,
  RSSI = 8,
  TRUSTED = 16,
  DEVICE_TYPE = 32,
};

class BluetoothManager {
private:
  AppContext *ctx;
  DBusMessage *devListMsg;
  
  unsigned char setDeviceProps(Device &dev, DBusMessageIter &propsIter);
  int getPropertyVal(const char *prop);

public:
  bool discovering, power;
  std::unordered_map<std::string, Device> devices;

  int getDeviceList();

  // Device Operations
  int connectDevice(bool state, std::string_view devPath);
  int trustDevice(bool state, std::string_view devPath);
  int removeDevice(std::string_view devPath);

  int switchDiscovery(bool on);
  int switchPower(bool on);
  
  // Monitor Changes Functions
  void addMatchRules();
  void handleInterfacesAdded(DBusMessageIter &rootIter);
  void handleInterfacesRemoved(DBusMessageIter &rootIter);
  void handlePropertiesChanged(DBusMessage *msg, DBusMessageIter &rootIter);

  BluetoothManager(AppContext *ctx);
  int setup();
};
