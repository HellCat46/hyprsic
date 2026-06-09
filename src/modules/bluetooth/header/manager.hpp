#pragma once

#include "services/header/comm_types.hpp"
#include "services/header/context.hpp"
#include "string"
#include "unordered_map"
#include <cstdint>
#include <memory>
#include <mutex>
#include <sdbus-c++/IObject.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Message.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

struct Device {
  std::string addr, name, path, deviceType;
  short rssi;
  bool paired, trusted, connected, mediaConnected;
  int16_t batteryPer;
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

  ModuleType moduleType;
  uint64_t correlationId;
};

struct BtTrustRequest {
  bool state;
  std::string_view devPath;

  ModuleType moduleType;
  uint64_t correlationId;
};

struct BtRemoveRequest {
  std::string_view devPath;

  ModuleType moduleType;
  uint64_t correlationId;
};

struct BtSwitchDiscoveryRequest {
  bool on;

  ModuleType moduleType;
  uint64_t correlationId;
};

struct BtSwitchPowerRequest {
  bool on;

  ModuleType moduleType;
  uint64_t correlationId;
};

using BtRequest =
    std::variant<BtConnectRequest, BtTrustRequest, BtRemoveRequest,
                 BtSwitchDiscoveryRequest, BtSwitchPowerRequest>;

class BluetoothManager {
  AppContext *ctx;
  std::unique_ptr<sdbus::IProxy> dbusProxy;
  std::unique_ptr<sdbus::IProxy> devListProxy;
  
  std::mutex devicesMtx;
  std::unordered_map<std::string, Device> devices;

  void setDeviceProps(Device &dev, const std::map<std::string, sdbus::Variant> &devData);
  int getPropertyBool(std::string prop, bool& result);

  // Device Operations
  ResponseMessage connectDevice(const BtConnectRequest &req);
  ResponseMessage trustDevice(const BtTrustRequest &req);
  ResponseMessage removeDevice(const BtRemoveRequest &req);
  ResponseMessage switchDiscovery(const BtSwitchDiscoveryRequest &req);
  ResponseMessage switchPower(const BtSwitchPowerRequest &req);

public:
  int updateDevList(); // Only for Manager Main Thread Use

  bool discovering, power;

  // Monitor Changes Functions
  void handleInterfacesAddedDbus(sdbus::Message &rootIter);
  void handleInterfacesRemovedDbus(sdbus::Message &rootIter);
  void handlePropertiesChangedDbus(sdbus::Message &msg);

  BluetoothManager(AppContext *ctx);
  int setup();

  std::unordered_map<std::string, Device> getDeviceList();

  ResponseMessage handle(const BtRequest &msg);
};
