#include "header/manager.hpp"
#include "cstring"
#include "services/header/comm_types.hpp"
#include "unordered_map"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <sdbus-c++/Error.h>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IObject.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/Types.h>
#include <string>
#include <unordered_map>

#define TAG "BluetoothManager"

BluetoothManager::BluetoothManager(AppContext *ctx) : ctx(ctx) {
  discovering = false;
  power = true;

  try {
    dbusProxy =
        sdbus::createProxy(*ctx->dbus.sysConn, sdbus::ServiceName{"org.bluez"},
                           sdbus::ObjectPath{"/org/bluez/hci0"});

    devListProxy =
        sdbus::createProxy(*ctx->dbus.sysConn, sdbus::ServiceName{"org.bluez"},
                           sdbus::ObjectPath{"/"});
  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG,
                         "Failed to create proxy: " + std::string(e.what()));
  }
}

int BluetoothManager::setup() {
  bool state;
  int res = getPropertyBool("Powered", state);
  if (res != -1) {
    std::string msg = "Initial Bluetooth Power State: ";
    msg += (state ? "ON" : "OFF");
    ctx->logger.LogInfo(TAG, msg);
    power = res;
  } else {
    ctx->logger.LogWarning(TAG, "Unable to get initial Bluetooth Power State. "
                                "Setting to ON by default.");
    power = true;
  }

  res = getPropertyBool("Discovering", state);
  if (res != -1) {
    std::string msg = "Initial Bluetooth Discovery State: ";
    msg += (state ? "ON" : "OFF");
    ctx->logger.LogInfo(TAG, msg);
    discovering = res;
  } else {
    ctx->logger.LogWarning(TAG,
                           "Unable to get initial Bluetooth Discovery State. "
                           "Setting to OFF by default.");
    discovering = false;
  }

  updateDevList();

  return 0;
}

ResponseMessage BluetoothManager::switchPower(const BtSwitchPowerRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  try {
    dbusProxy->callMethod(sdbus::MethodName{"Set"})
        .onInterface(sdbus::InterfaceName{"org.freedesktop.DBus.Properties"})
        .withArguments(std::string{"org.bluez.Adapter1"},
                       std::string{"Powered"}, sdbus::Variant{req.on});

    std::string logMsg = "Turned ";
    logMsg += (req.on ? "ON" : "OFF");
    logMsg += " Bluetooth Power.";
    ctx->logger.LogInfo(TAG, logMsg);

    this->power = req.on;
  } catch (const sdbus::Error &e) {
    resp.errMsg = "Failed to set power state: " + std::string(e.what());
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  resp.success = true;
  return resp;
}

ResponseMessage
BluetoothManager::switchDiscovery(const BtSwitchDiscoveryRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  try {
    dbusProxy
        ->callMethod(
            sdbus::MethodName{req.on ? "StartDiscovery" : "StopDiscovery"})
        .onInterface(sdbus::InterfaceName{"org.bluez.Adapter1"});

    std::string logMsg = "Turning ";
    logMsg += (req.on ? "ON" : "OFF");
    logMsg += " Bluetooth Discovery.";
    ctx->logger.LogInfo(TAG, logMsg);
    this->discovering = req.on;
  } catch (const sdbus::Error &e) {
    resp.errMsg = "Failed to switch discovery: " + std::string(e.what());
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  resp.success = true;
  return resp;
}

void BluetoothManager::handleInterfacesAddedDbus(sdbus::Message &msg) {

  try {
    // Getting Object Path
    std::string path;
    msg >> path;

    // a{sa{sv}}
    std::map<std::string, std::map<std::string, sdbus::Variant>> props;
    msg >> props;

    if (props.contains("org.bluez.Device1")) {
      auto devData = props.at("org.bluez.Device1");
      Device dev{"", "", path, "", -110, false, false, false, false, -1};

      setDeviceProps(dev, devData);

      devices.insert({path, dev});
      std::string logMsg = "Added Device to Device List. Total Devices: ";
      logMsg += std::to_string(devices.size());
      ctx->logger.LogInfo(TAG, logMsg);
    }
  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG, "Failed to handle InterfacesAdded: " +
                                  std::string(e.what()));
    return;
  }
}

void BluetoothManager::handleInterfacesRemovedDbus(sdbus::Message &msg) {
  try {
    std::string path;
    msg >> path;
    if (devices.find(path) != devices.end()) {
      devices.erase(path);
      std::string logMsg = "Removed Device from Device List. Total Devices: ";
      logMsg += std::to_string(devices.size());
      ctx->logger.LogInfo(TAG, logMsg);
    } else {
      ctx->logger.LogWarning(TAG,
                             "Unable to find Device in Device List. Skipping.");
    }
  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG, "Failed to handle InterfacesRemoved: " +
                                  std::string(e.what()));
    return;
  }
}

void BluetoothManager::handlePropertiesChangedDbus(sdbus::Message &msg) {
  try {
    std::string path = msg.getPath();
    if (path.size() > 37)
      return;

    // Extract Device Address from Object Path
    path = path.substr(path.find("dev_") + 4);
    if (path.size() < 17) {
      ctx->logger.LogWarning(
          TAG, "Unable to Extract Device Address from Object Path. "
               "Unknown Format. Skipping.");
      return;
    }
    std::replace(path.begin(), path.end(), '_', ':');

    auto dev = devices.find(path);
    if (dev == devices.end()) {
      std::string errMsg = "Unable to find Device in Device List. Skipping. ";
      errMsg += path;
      ctx->logger.LogError(TAG, errMsg);
      return;
    }

    std::string iface;
    msg >> iface;

    if (iface != "org.bluez.Device1") {
      return;
    }

    std::map<std::string, sdbus::Variant> devData;
    msg >> devData;

    bool prevConnected = dev->second.connected;
    setDeviceProps(dev->second, devData);

    if (prevConnected != dev->second.connected) {
      if (dev->second.connected) {
        ctx->showUpdateWindow(dev->second.deviceType == "audio-headset"
                                  ? "audio-headset-symbolic"
                                  : "bluetooth-active-symbolic",
                              "Connected to Device: " + dev->second.name);
      } else {
        ctx->showUpdateWindow("bluetooth-disconnected-symbolic",
                              "Disconnected from Device: " + dev->second.name);
      }
    }

    std::string updateMsg = "Updated Device Properties. Total Devices: " +
                            std::to_string(devices.size());
    ctx->logger.LogInfo(TAG, updateMsg);
  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG, "Failed to handle PropertiesChanged: " +
                                  std::string(e.what()));
    return;
  }
}

int BluetoothManager::updateDevList() {
  devices.clear();

  try {

    std::map<sdbus::ObjectPath,
             std::map<std::string, std::map<std::string, sdbus::Variant>>>
        data;
    devListProxy->callMethod(sdbus::MethodName{"GetManagedObjects"})
        .onInterface(sdbus::InterfaceName{"org.freedesktop.DBus.ObjectManager"})
        .storeResultsTo(data);

    for (const auto &[objPath, ifaceData] : data) {
      if (objPath.find("/org/bluez/hci0/dev") != std::string::npos) {
        Device dev{"", "", objPath, "", -110, false, false, false, false, -1};

        if (ifaceData.find("org.bluez.Device1") != ifaceData.end()) {
          const auto &props = ifaceData.at("org.bluez.Device1");
          setDeviceProps(dev, props);
        }

        if (ifaceData.find("org.bluez.Battery1") != ifaceData.end()) {
          const auto &props = ifaceData.at("org.bluez.Battery1");
          if (props.find("Percentage") != props.end()) {
            dev.batteryPer = props.at("Percentage").get<uint8_t>();
          }
        }

        if (ifaceData.find("org.bluez.MediaControl1") != ifaceData.end()) {
          const auto &props = ifaceData.at("org.bluez.MediaControl1");
          if (props.find("MediaConnected") != props.end()) {
            dev.mediaConnected = props.at("MediaConnected").get<bool>();
          }
        }

        if(dev.addr.empty()) {
          continue;
        }

        // ctx->logger.LogInfo(TAG, "Adding device: " + dev.name + " (" + dev.addr + ")");
        devices.insert({dev.addr, dev});
      }
    }

  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG, "Failed to update Device List: " +
                                  std::string(e.what()));
    return 1;
  }
  return 0;
}

std::unordered_map<std::string, Device> BluetoothManager::getDeviceList() {
  std::lock_guard<std::mutex> lock(devicesMtx);

  return devices;
}

int BluetoothManager::getPropertyBool(std::string prop, bool &result) {

  try {
    sdbus::Variant variant;
    dbusProxy->callMethod(sdbus::MethodName{"Get"})
        .onInterface(sdbus::InterfaceName{"org.freedesktop.DBus.Properties"})
        .withArguments(std::string{"org.bluez.Adapter1"}, prop)
        .storeResultsTo(variant);

    result = variant.get<bool>();

  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG,
                         "Failed to get property: " + std::string(e.what()));
    return -1;
  }

  return 0;
}

ResponseMessage BluetoothManager::connectDevice(const BtConnectRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  std::string logMsg = req.state ? "Connecting" : "Disconnecting";
  logMsg += " to Device: ";
  logMsg += req.devPath;
  ctx->logger.LogInfo(TAG, logMsg);

  try {
    auto proxy =
        sdbus::createProxy(*ctx->dbus.sysConn, sdbus::ServiceName{"org.bluez"},
                           sdbus::ObjectPath{std::string(req.devPath)});

    proxy->callMethod(sdbus::MethodName{req.state ? "Connect" : "Disconnect"})
        .onInterface(sdbus::InterfaceName{"org.bluez.Device1"});

    std::string successMsg = req.state ? "Connected" : "Disconnected";
    successMsg += " to Device: ";
    successMsg += req.devPath;
    ctx->logger.LogInfo(TAG, successMsg);
  } catch (const sdbus::Error &e) {
    resp.errMsg =
        "Failed to Connect/Disconnect to Device: " + std::string(e.what());
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  resp.success = true;
  return resp;
}

ResponseMessage BluetoothManager::trustDevice(const BtTrustRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  std::string logMsg = "Trying to ";
  logMsg += req.state ? "Trust" : "Untrust";
  logMsg += " Device: ";
  logMsg += req.devPath;
  ctx->logger.LogInfo(TAG, logMsg);

  size_t pos = req.devPath.find('_');
  if (pos == std::string_view::npos) {
    resp.errMsg = "Invalid Device Path Format: ";
    resp.errMsg += req.devPath;
    ctx->logger.LogError(TAG, resp.errMsg);

    return resp;
  }

  std::string path = std::string(req.devPath.substr(pos + 1));
  std::replace(path.begin(), path.end(), '_', ':');

  const auto &devIt = devices.find(path);
  if (devIt == devices.end()) {
    resp.errMsg = "Device not found in Device List: ";
    resp.errMsg += req.devPath;
    ctx->logger.LogError(TAG, resp.errMsg);

    return resp;
  }

  try {
    auto proxy =
        sdbus::createProxy(*ctx->dbus.sysConn, sdbus::ServiceName{"org.bluez"},
                           sdbus::ObjectPath{std::string(req.devPath)});

    proxy->callMethod(sdbus::MethodName{"Set"})
        .onInterface(sdbus::InterfaceName{"org.freedesktop.DBus.Properties"})
        .withArguments(std::string{"org.bluez.Device1"}, std::string{"Trusted"},
                       sdbus::Variant(!devIt->second.trusted));

    std::string successMsg = req.state ? "Trusted" : "Untrusted";
    successMsg += " Device: ";
    successMsg += req.devPath;
    ctx->logger.LogInfo(TAG, successMsg);
  } catch (const sdbus::Error &e) {
    resp.errMsg = "Failed to Trust/Remove Device: " + std::string(e.what());
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  resp.success = true;
  return resp;
}

ResponseMessage BluetoothManager::removeDevice(const BtRemoveRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};
  ctx->logger.LogInfo(TAG,
                      "Trying to Remove Device: " + std::string(req.devPath));

  try {

    dbusProxy->callMethod(sdbus::MethodName{"RemoveDevice"})
        .onInterface(sdbus::InterfaceName{"org.bluez.Adapter1"})
        .withArguments(sdbus::ObjectPath{std::string(req.devPath)});

    ctx->logger.LogInfo(TAG, "Removed Device: " + std::string(req.devPath));
  } catch (const sdbus::Error &e) {
    resp.errMsg = "Failed to Trust/Remove Device: " + std::string(e.what());
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  resp.success = true;
  return resp;
}

void BluetoothManager::setDeviceProps(
    Device &dev, const std::map<std::string, sdbus::Variant> &devData) {

  if (devData.contains("Name")) {
    dev.name = devData.at("Name").get<std::string>();
  }
  if (devData.contains("Address")) {
    dev.addr = devData.at("Address").get<std::string>();
  }
  if (devData.contains("RSSI")) {
    dev.rssi = devData.at("RSSI").get<int16_t>();
  }
  if (devData.contains("Paired")) {
    dev.paired = devData.at("Paired").get<bool>();
  }
  if (devData.contains("Trusted")) {
    dev.trusted = devData.at("Trusted").get<bool>();
  }
  if (devData.contains("Connected")) {
    dev.connected = devData.at("Connected").get<bool>();
  }
  if (devData.contains("Icon")) {
    dev.deviceType = devData.at("Icon").get<std::string>();
  }
}

ResponseMessage BluetoothManager::handle(const BtRequest &req) {
  ResponseMessage resp;

  std::visit(
      [&](auto &reqMsg) {
        using T = std::decay_t<decltype(reqMsg)>;

        if constexpr (std::is_same_v<T, BtConnectRequest>) {
          resp = connectDevice(reqMsg);
        } else if constexpr (std::is_same_v<T, BtTrustRequest>) {
          resp = trustDevice(reqMsg);
        } else if constexpr (std::is_same_v<T, BtRemoveRequest>) {
          resp = removeDevice(reqMsg);
        } else if constexpr (std::is_same_v<T, BtSwitchDiscoveryRequest>) {
          resp = switchDiscovery(reqMsg);
        } else if constexpr (std::is_same_v<T, BtSwitchPowerRequest>) {
          resp = switchPower(reqMsg);
        }
      },
      req);

  return resp;
}
