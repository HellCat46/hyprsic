#pragma once

#include "cstring"
#include "dbus/dbus.h"
#include "services/header/comm_types.hpp"
#include "services/header/context.hpp"
#include "string"
#include "unordered_map"
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

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

struct BtConnectRequest {
  bool state;
  std::string_view devPath;

  uint64_t correlationId;
};

struct BtTrustRequest {
  bool state;
  std::string_view devPath;

  uint64_t correlationId;
};

struct BtRemoveRequest {
  std::string_view devPath;

  uint64_t correlationId;
};

struct BtSwitchDiscoveryRequest {
  bool on;

  uint64_t correlationId;
};

struct BtSwitchPowerRequest {
  bool on;

  uint64_t correlationId;
};

using BtRequest =
    std::variant<BtConnectRequest, BtTrustRequest, BtRemoveRequest,
                 BtSwitchDiscoveryRequest, BtSwitchPowerRequest>;
class BluetoothManager {
private:
  AppContext *ctx;
  DBusMessage *devListMsg;

  unsigned char setDeviceProps(Device &dev, DBusMessageIter &propsIter);
  int getPropertyVal(const char *prop);

  // Device Operations
  ResponseMessage connectDevice(const BtConnectRequest &req);
  ResponseMessage trustDevice(const BtTrustRequest &req);
  ResponseMessage removeDevice(const BtRemoveRequest &req);
  ResponseMessage switchDiscovery(const BtSwitchDiscoveryRequest &req);
  ResponseMessage switchPower(const BtSwitchPowerRequest &req);

public:
  int getDeviceList(); // Only for Manager Main Thread Use

  bool discovering, power;
  std::unordered_map<std::string, Device> devices;

  // Monitor Changes Functions
  void addMatchRulesDbus();
  void handleInterfacesAddedDbus(DBusMessageIter &rootIter);
  void handleInterfacesRemovedDbus(DBusMessageIter &rootIter);
  void handlePropertiesChangedDbus(DBusMessage *msg, DBusMessageIter &rootIter);

  BluetoothManager(AppContext *ctx);
  int setup();

  ResponseMessage handle(const BtRequest &msg);
};
