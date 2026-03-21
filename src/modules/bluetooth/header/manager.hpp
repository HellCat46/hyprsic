#pragma once

#include "services/header/comm_types.hpp"
#include "services/header/context.hpp"
#include "cstring"
#include "dbus/dbus.h"
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
    
    long int correlationId;
};

struct BtTrustRequest {
    bool state;
    std::string_view devPath;
    
    long int correlationId;
};

struct BtRemoveRequest {
    std::string_view devPath;
    
    long int correlationId;
};

struct BtSwitchDiscoveryRequest {
    bool on;
    
    long int correlationId;
};

struct BtSwitchPowerRequest {
    bool on;
    
    long int correlationId;
};


using BtRequest = std::variant<BtConnectRequest, BtTrustRequest, BtRemoveRequest, BtSwitchDiscoveryRequest, BtSwitchPowerRequest>;
class BluetoothManager {
private:
  AppContext *ctx;
  DBusMessage *devListMsg;
  
  unsigned char setDeviceProps(Device &dev, DBusMessageIter &propsIter);
  int getPropertyVal(const char *prop);
  
  
  // Device Operations
  ResponseMessage connectDevice(BtConnectRequest req);
  ResponseMessage trustDevice(BtTrustRequest req);
  ResponseMessage removeDevice(BtRemoveRequest req);
  ResponseMessage switchDiscovery(BtSwitchDiscoveryRequest req);
  ResponseMessage switchPower(BtSwitchPowerRequest req);
  
  int getDeviceList();
public:

  bool discovering, power;
  std::unordered_map<std::string, Device> devices;


  
  // Monitor Changes Functions
  void addMatchRulesDbus();
  void handleInterfacesAddedDbus(DBusMessageIter &rootIter);
  void handleInterfacesRemovedDbus(DBusMessageIter &rootIter);
  void handlePropertiesChangedDbus(DBusMessage *msg, DBusMessageIter &rootIter);

  BluetoothManager(AppContext *ctx);
  int setup();
  
  ResponseMessage handle(const BtRequest& msg);
};
