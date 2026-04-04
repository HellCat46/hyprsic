#pragma once

#include "dbus/dbus.h"
#include "services/header/comm_types.hpp"
#include "services/header/context.hpp"
#include <string>
#include <unordered_map>
#include <variant>

struct WifiScanRequest {
  uint64_t correlationId;
};

struct WifiConnectRequest {
  std::string netPath;

  uint64_t correlationId;
};

struct WifiDisconnectRequest {
  uint64_t correlationId;
};

struct WifiForgetRequest {
  std::string netPath;
  
  uint64_t correlationId;
};

struct WifiSubmitPassphraseRequest {
  std::string password;
  
  uint64_t correlationId;
};

struct WifiStation {
  std::string ssid, type;
  bool known, autoConn;
  short rssi;
};


using WifiRequest = std::variant<WifiConnectRequest, WifiDisconnectRequest, WifiForgetRequest, WifiScanRequest, WifiSubmitPassphraseRequest>;
class WifiManager {
  AppContext *ctx;
  std::string devPath, devAddr, devName, devAdapter, agentPath;
  bool powered = false, scanning = false;
  std::string connDev, authDev;
  DBusMessage *authMsg;

  void RegisterAgent(bool reg);
  void GetManagedObjects();

  int GetConnectedDevice();
  void GetDevices();
  int GetDeviceInfo(std::string devPath, WifiStation &station);

  // Action methods
  ResponseMessage Scan(const WifiScanRequest& req);
  ResponseMessage Connect(const WifiConnectRequest& req);
  ResponseMessage Disconnect(const WifiDisconnectRequest& req);
  ResponseMessage Forget(const WifiForgetRequest& req);
  ResponseMessage SubmitPassphrase(const WifiSubmitPassphraseRequest& req);

public:
  std::unordered_map<std::string, WifiStation> devices;

  WifiManager(AppContext *appCtx);
  ~WifiManager();
  void update(bool force = false);

  bool IsPowered() const;
  bool IsScanning() const;
  
  std::string getConnDev() const;
  std::string getAuthDev() const;

  // Monitor Changes Functions
  void addMatchRulesDbus();
  void handleRequestPassphraseDbus(DBusMessage *msg, DBusMessageIter &rootIter);
  void handleRequestCancelDbus();
  void handleInterfacesRemovedDbus(DBusMessageIter &rootIter);
  void handlePropertiesChangedDbus(DBusMessage *msg, DBusMessageIter &rootIter);
  
  ResponseMessage handle(const WifiRequest& msg);
};
