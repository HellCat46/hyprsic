#pragma once
#include "../app/context.hpp"
#include "cstring"
#include "dbus/dbus.h"
#include "string"
#include <string>
#include <unordered_map>

struct ConnectedDevice {
  std::string iface, addr, name, icon;
  bool connected, mediaConnected;
  short batteryPer;
};

class BluetoothDevice {
private:
  AppContext *ctx;
  DBusMessage *msg;

public:
  std::unordered_map<std::string, ConnectedDevice> connectedDev;
  std::unordered_map<std::string, ConnectedDevice> availDev;
  BluetoothDevice(AppContext *context);
  int getDeviceList();
  bool printDevicesInfo();
};
