#pragma once

#include "services/header/comm_types.hpp"
#include "services/header/context.hpp"
#include <memory>
#include <sdbus-c++/Message.h>
#include <string>
#include <unordered_map>
#include <variant>

struct WifiScanRequest {
  ModuleType moduleType;
  uint64_t correlationId;
};

struct WifiConnectRequest {
  std::string netPath;

  ModuleType moduleType;
  uint64_t correlationId;
};

struct WifiDisconnectRequest {
  ModuleType moduleType;
  uint64_t correlationId;
};

struct WifiForgetRequest {
  std::string netPath;

  ModuleType moduleType;
  uint64_t correlationId;
};

struct WifiSubmitPassphraseRequest {
  std::string password;

  ModuleType moduleType;
  uint64_t correlationId;
};

struct WifiStation {
  std::string ssid, type;
  bool known, autoConn;
  short rssi;
};

using WifiRequest =
    std::variant<WifiConnectRequest, WifiDisconnectRequest, WifiForgetRequest,
                 WifiScanRequest, WifiSubmitPassphraseRequest>;

class WifiManager {
  AppContext *ctx;
  Glib::RefPtr<sdbus::IObject> dbusObj;
  std::unique_ptr<sdbus::IProxy> dbusProxy;

  std::string devAddr, devName, devAdapter, agentPath;
  bool powered = false, scanning = false;
  std::string connDev, authDev;
  std::unique_ptr<sdbus::MethodCall> authMsg;

  void RegisterAgent(bool reg);
  void GetManagedObjects();

  int GetConnectedDevice();
  void GetDevices();
  int GetDeviceInfo(std::string devPath, WifiStation &station);

  void clearDevicePath(std::string &devPath);

  // Action methods
  ResponseMessage Scan(const WifiScanRequest &req);
  ResponseMessage Connect(const WifiConnectRequest &req);
  ResponseMessage Disconnect(const WifiDisconnectRequest &req);
  ResponseMessage Forget(const WifiForgetRequest &req);
  ResponseMessage SubmitPassphrase(const WifiSubmitPassphraseRequest &req);

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
  void handleRequestPassphraseDbus(sdbus::MethodCall msg);
  void handleRequestCancelDbus();
  void handleInterfacesRemovedDbus(sdbus::Message &msg);
  void handlePropertiesChangedDbus(sdbus::Message &msg);

  ResponseMessage handle(const WifiRequest &msg);
};
