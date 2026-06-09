#include "header/manager.hpp"
#include "services/header/comm_types.hpp"
#include <cstddef>
#include <cstring>
#include <map>
#include <memory>
#include <sdbus-c++/Error.h>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/VTableItems.h>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>

#define TAG "WifiManager"

WifiManager::WifiManager(AppContext *appCtx)
    : ctx(appCtx), agentPath("/agent/" + std::to_string(getpid())),
      authMsg(nullptr) {

  RegisterAgent(true);
  GetManagedObjects();
  
  try {
    dbusProxy = sdbus::createProxy(*ctx->dbus.sysConn,
                                   sdbus::ServiceName{"net.connman.iwd"},
                                   sdbus::ObjectPath{devAdapter});
    ctx->logger.LogInfo(TAG, "Successfully created D-Bus proxy for Adapter: " + devAdapter);

  } catch (const std::exception &e) {
    ctx->logger.LogError(TAG, "Failed to create D-Bus proxy: " +
                                  std::string{e.what()});
    return;
  }

  GetConnectedDevice();
  GetDevices();
  update();
}

void WifiManager::update(bool force) {
  if (force || scanning)
    GetDevices();
}

void WifiManager::RegisterAgent(bool reg) {
  try {
    auto proxy = sdbus::createProxy(*ctx->dbus.sysConn,
                                    sdbus::ServiceName{"net.connman.iwd"},
                                    sdbus::ObjectPath{"/net/connman/iwd"});

    proxy
        ->callMethod(
            sdbus::MethodName{reg ? "RegisterAgent" : "UnregisterAgent"})
        .onInterface(sdbus::InterfaceName{"net.connman.iwd.AgentManager"})
        .withArguments(sdbus::ObjectPath{agentPath});
  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG, std::string{"D-Bus RegisterAgent call failed: "} +
                                  e.what());
    return;
  }
}

void WifiManager::GetManagedObjects() {
  try {
    auto proxy = sdbus::createProxy(*ctx->dbus.sysConn,
                                    sdbus::ServiceName{"net.connman.iwd"},
                                    sdbus::ObjectPath{"/"});

    // (a{oa{sa{sv}}})
    std::map<sdbus::ObjectPath,
             std::map<std::string, std::map<std::string, sdbus::Variant>>>
        data;

    proxy->callMethod(sdbus::MethodName{"GetManagedObjects"})
        .onInterface(sdbus::InterfaceName{"org.freedesktop.DBus.ObjectManager"})
        .storeResultsTo(data);

    for (const auto &[path, ifaces] : data) {

      if (ifaces.contains("net.connman.iwd.Device")) {
        auto ifaceData = ifaces.at("net.connman.iwd.Device");

        if (ifaceData.contains("Powered")) {
          powered = ifaceData.at("Powered").get<bool>();
        }

        if (ifaceData.contains("Address")) {
          devAddr = ifaceData.at("Address").get<std::string>();
        }

        if (ifaceData.contains("Name")) {
          devName = ifaceData.at("Name").get<std::string>();
        }

        devAdapter = path;
      }

      if (ifaces.contains("net.connman.iwd.KnownNetwork")) {
        auto ifaceData = ifaces.at("net.connman.iwd.KnownNetwork");

        WifiStation dev;
        if (ifaceData.contains("Name")) {
          dev.ssid = ifaceData.at("Name").get<std::string>();
        }
        if (ifaceData.contains("Type")) {
          dev.type = ifaceData.at("Type").get<std::string>();
        }
        if (ifaceData.contains("AutoConnect")) {
          dev.autoConn = ifaceData.at("AutoConnect").get<bool>();
        }

        dev.known = true;
        dev.rssi = -100;

        size_t pos = path.rfind("/");
        if (pos != std::string_view::npos) {

          devices.insert({path.substr(pos + 1), dev});
        }
      }
    }

  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(
        TAG, std::string{"D-Bus GetManagedObjects call failed: "} + e.what());
    return;
  }

  ctx->logger.LogInfo(TAG,
                      "Successfully retrieved Device Adapter: " + devAdapter +
                          ", Address: " + devAddr + ", Name: " + devName);
  ctx->logger.LogInfo(TAG, "Known Networks Count: " +
                               std::to_string(devices.size()));
}

ResponseMessage WifiManager::Scan(const WifiScanRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  try {
    dbusProxy->callMethod(sdbus::MethodName{"Scan"})
        .onInterface("net.connman.iwd.Station");
  } catch (const std::exception &e) {
    resp.errMsg = "Scan failed: " + std::string{e.what()};
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  scanning = true;
  resp.success = true;
  return resp;
}

int WifiManager::GetConnectedDevice() {

  try {
    sdbus::Variant deviceVar;
    dbusProxy->callMethod(sdbus::MethodName{"Get"})
        .onInterface(sdbus::InterfaceName{"org.freedesktop.DBus.Properties"})
        .withArguments(std::string{"net.connman.iwd.Station"},
                       std::string{"ConnectedNetwork"})
        .storeResultsTo(deviceVar);

    connDev = deviceVar.get<sdbus::ObjectPath>();
  } catch (const std::exception &e) {
    ctx->logger.LogError(TAG,
                         "GetConnectedDevice failed: " + std::string{e.what()});
    return -1;
  }

  return 0;
}

void WifiManager::GetDevices() {
  try {

    std::vector<sdbus::Struct<sdbus::ObjectPath, int16_t>> netList;
    dbusProxy->callMethod(sdbus::MethodName{"GetOrderedNetworks"})
        .onInterface("net.connman.iwd.Station")
        .storeResultsTo(netList);

    for (const auto &network : netList) {
      WifiStation station;
      std::string netPath = network.get<0>();
      size_t pos = netPath.rfind("/");
      if (pos != std::string_view::npos) {
        netPath = netPath.substr(pos + 1);
      }
      
      station.rssi = network.get<1>() / 100;

      auto it = devices.find(netPath);
      if (it != devices.end()) {
        it->second.rssi = station.rssi;
        continue;
      }

      if (!GetDeviceInfo(netPath, station)) {
        station.known = false;
      }
      devices.insert({netPath, station});
    }

  } catch (const std::exception &e) {
    ctx->logger.LogError(TAG, "GetDevices failed: " + std::string{e.what()});
  }
}

int WifiManager::GetDeviceInfo(std::string dev, WifiStation &station) {
  try {
    std::unique_ptr<sdbus::IProxy> proxy = sdbus::createProxy(
        *ctx->dbus.sysConn, sdbus::ServiceName{"net.connman.iwd"},
        sdbus::ObjectPath{devAdapter + "/" + dev});

    std::map<std::string, sdbus::Variant> devData;
    proxy->callMethod(sdbus::MethodName{"GetAll"})
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments(std::string{"net.connman.iwd.Network"})
        .storeResultsTo(devData);

    if (devData.contains("Name")) {
      station.ssid = devData.at("Name").get<std::string>();
    }

    if (devData.contains("Connected")) {
      bool connected = devData.at("Connected").get<bool>();
      if (connected) {
        connDev = dev;
      }
    }

    if (devData.contains("Type")) {
      station.type = devData.at("Type").get<std::string>();
    }

  } catch (const std::exception &e) {
    ctx->logger.LogError(TAG, "GetDeviceInfo failed: " + std::string{e.what()});
    return -1;
  }

  return 0;
}

bool WifiManager::IsPowered() const { return powered; }
bool WifiManager::IsScanning() const { return scanning; }

ResponseMessage WifiManager::Connect(const WifiConnectRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  std::string path = devAdapter + "/" + req.netPath;

  try {
    std::unique_ptr<sdbus::IProxy> proxy = sdbus::createProxy(
        *ctx->dbus.sysConn, sdbus::ServiceName{"net.connman.iwd"},
        sdbus::ObjectPath{path});

    proxy->callMethod(sdbus::MemberName{"Connect"})
        .onInterface(sdbus::InterfaceName{"net.connman.iwd.Network"});
  } catch (const sdbus::Error &e) {
    resp.errMsg = "D-Bus Connect call failed: " + std::string{e.what()};
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  resp.success = true;
  return resp;
}

ResponseMessage WifiManager::Disconnect(const WifiDisconnectRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  try {
    dbusProxy->callMethod(sdbus::MemberName{"Disconnect"})
        .onInterface(sdbus::InterfaceName{"net.connman.iwd.Station"});
  } catch (const sdbus::Error &e) {
    resp.errMsg = "D-Bus Connect call failed: " + std::string{e.what()};
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  resp.success = true;
  return resp;
}

ResponseMessage WifiManager::Forget(const WifiForgetRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  std::string path = "/net/connman/iwd/" + req.netPath;

  try {
    std::unique_ptr<sdbus::IProxy> proxy = sdbus::createProxy(
        *ctx->dbus.sysConn, sdbus::ServiceName{"net.connman.iwd"},
        sdbus::ObjectPath{path});

    proxy->callMethod(sdbus::MemberName{"Forget"})
        .onInterface(sdbus::InterfaceName{"net.connman.iwd.KnownNetwork"});
  } catch (const sdbus::Error &e) {
    resp.errMsg = "D-Bus Connect call failed: " + std::string{e.what()};
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  resp.success = true;
  return resp;
}

ResponseMessage
WifiManager::SubmitPassphrase(const WifiSubmitPassphraseRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  if (authMsg == nullptr || authDev.empty()) {
    resp.errMsg = "The Connection has been cancelled";
    return resp;
  }

  try {
    sdbus::MethodReply reply = authMsg->createReply();
    reply << req.password;
    reply.send();

    ctx->logger.LogInfo(TAG, "Submitted passphrase for device: " + authDev);
    authMsg = nullptr;
    authDev = "";
  } catch (const sdbus::Error &e) {
    resp.errMsg = "Failed to create reply for Passphrase Method Response";
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  resp.success = true;
  return resp;
}

void WifiManager::addMatchRulesDbus() {
  ctx->logger.LogInfo(TAG, "Adding D-Bus Match Rules for WifiManager");

  try {
    dbusObj =
        sdbus::createObject(*ctx->dbus.sysConn, sdbus::ObjectPath{agentPath});

    dbusObj
        ->addVTable(
            sdbus::MethodVTableItem{sdbus::MethodName{"RequestPassphrase"},
                                    sdbus::Signature{"s"},
                                    {},
                                    {},
                                    {},
                                    [this](sdbus::MethodCall msg) {
                                      handleRequestPassphraseDbus(msg);
                                    },
                                    {}},
            sdbus::MethodVTableItem{
                sdbus::MethodName{"Cancel"},
                sdbus::Signature{""},
                {},
                {},
                {},
                [this](sdbus::MethodCall _) { handleRequestCancelDbus(); },
                {}})
        .forInterface("net.connman.iwd.Agent");

  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG, "Failed to add D-Bus match rules: " +
                                  std::string(e.what()));
    return;
  }

  ctx->logger.LogInfo(TAG,
                      "Successfully added D-Bus match rules for WifiManager");
}

void WifiManager::handleRequestPassphraseDbus(sdbus::MethodCall msg) {
  std::string ssid;
  msg >> ssid;
  size_t pos = ssid.rfind("/");
  if (pos != std::string_view::npos) {
    authMsg = std::make_unique<sdbus::MethodCall>(msg);
    authDev = ssid.substr(pos + 1);
  }
}

void WifiManager::handleRequestCancelDbus() {
  authDev = "";
  if (authMsg) {
    authMsg.reset();
  }
}

void WifiManager::handleInterfacesRemovedDbus(sdbus::Message &msg) {
  std::string objPath;

  try {
    msg >> objPath;
    size_t pos = objPath.rfind("/");
    if (pos != std::string_view::npos) {
      objPath = objPath.substr(pos + 1);
    }

    devices.erase(objPath);
    if (connDev == objPath) {
      connDev = "";
    }
  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG, "Failed to parse InterfacesRemoved signal: " +
                                  std::string(e.what()));
  }
}

void WifiManager::handlePropertiesChangedDbus(sdbus::Message &msg) {
  std::string iface;

  try {
    msg >> iface;

    if (iface == "net.connman.iwd.Station") {
      std::map<std::string, sdbus::Variant> props;
      msg >> props;

      if (props.contains("Scanning")) {
        this->scanning = props["Scanning"].get<bool>();
      }
      if (props.contains("ConnectedNetwork")) {
        std::string objPath = props["ConnectedNetwork"].get<std::string>();
        size_t pos = objPath.rfind("/");
        if (pos != std::string_view::npos) {
          objPath = objPath.substr(pos + 1);
        }

        connDev = objPath;
      }
    } else if (iface == "net.connman.iwd.Network") {
      std::map<std::string, sdbus::Variant> props;
      msg >> props;

      if (props.contains("Connected")) {
        bool connected = props["Connected"].get<bool>();

        if (connected) {
          std::string path = msg.getPath();
          size_t pos = path.rfind("/");
          if (pos != std::string_view::npos) {
            path = path.substr(pos + 1);
          }
          connDev = path;
        }
      }
    }

  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG, "Failed to parse PropertiesChanged signal: " +
                                  std::string(e.what()));
    return;
  }
}

std::string WifiManager::getAuthDev() const { return authDev; }

std::string WifiManager::getConnDev() const { return connDev; }

WifiManager::~WifiManager() { RegisterAgent(false); }

ResponseMessage WifiManager::handle(const WifiRequest &req) {
  ResponseMessage resp;

  std::visit(
      [&](auto &reqMsg) {
        using T = std::decay_t<decltype(reqMsg)>;

        if constexpr (std::is_same_v<T, WifiConnectRequest>) {
          resp = Connect(reqMsg);
        } else if constexpr (std::is_same_v<T, WifiDisconnectRequest>) {
          resp = Disconnect(reqMsg);
        } else if constexpr (std::is_same_v<T, WifiForgetRequest>) {
          resp = Forget(reqMsg);
        } else if constexpr (std::is_same_v<T, WifiScanRequest>) {
          resp = Scan(reqMsg);
        } else if constexpr (std::is_same_v<T, WifiSubmitPassphraseRequest>) {
          resp = SubmitPassphrase(reqMsg);
        }
      },
      req);

  return resp;
}
