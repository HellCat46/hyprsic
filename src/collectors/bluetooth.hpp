#pragma once
#include "../app/context.hpp"
#include "cstring"
#include "string"
#include "dbus/dbus.h"
#include "vector"

struct ConnectedDevice {
  std::string iface, addr, name, icon;
  bool connected, mediaConnected;
  short batteryPer;
};

class BluetoothDevice {
private:
  AppContext *ctx;
  DBusMessage *msg;
  std::vector<ConnectedDevice> devices;

public:
  BluetoothDevice(AppContext *context);
  int getDeviceList();
  bool printDevicesInfo();
};
