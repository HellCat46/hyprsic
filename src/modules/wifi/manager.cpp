#include "manager.hpp"
#include "../../utils/helper_func.hpp"
#include "dbus/dbus-protocol.h"
#include "dbus/dbus.h"
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <unistd.h>

#define TAG "WifiManager"

WifiManager::WifiManager(AppContext *appCtx)
    : ctx(appCtx), agentPath("/agent/" + std::to_string(getpid())),
      authMsg(nullptr) {

  RegisterAgent(true);
  GetManagedObjects();
  GetConnectedDevice();
  GetDevices();

  update();
}

void WifiManager::update(bool force) {
  if (force || scanning)
    GetDevices();
}

void WifiManager::RegisterAgent(bool reg) {
  DBusMessage *msg = dbus_message_new_method_call(
      "net.connman.iwd", "/net/connman/iwd", "net.connman.iwd.AgentManager",
      reg ? "RegisterAgent" : "UnregisterAgent");
  if (!msg) {
    ctx->logger.LogError(TAG,
                         "Failed to create D-Bus message for RegisterAgent");
    return;
  }

  DBusMessageIter args;
  dbus_message_iter_init_append(msg, &args);

  const char *path = agentPath.c_str();
  dbus_message_iter_append_basic(&args, DBUS_TYPE_OBJECT_PATH, &path);

  dbus_message_iter_init_closed(&args);

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, msg, -1, &ctx->dbus.sysErr);
  if (dbus_error_is_set(&ctx->dbus.sysErr) && !reply) {
    ctx->logger.LogError(TAG, std::string("D-Bus RegisterAgent call failed: ") +
                                  ctx->dbus.sysErr.message);
    dbus_error_free(&ctx->dbus.sysErr);
    dbus_message_unref(msg);
    return;
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);
}

void WifiManager::GetManagedObjects() {

  DBusMessage *msg = dbus_message_new_method_call(
      "net.connman.iwd", "/", "org.freedesktop.DBus.ObjectManager",
      "GetManagedObjects");
  if (!msg) {
    ctx->logger.LogError(TAG, "Failed to create D-Bus message for ListNames");
    return;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, msg, -1, &ctx->dbus.sysErr);
  if (dbus_error_is_set(&ctx->dbus.sysErr) && !reply) {
    ctx->logger.LogError(TAG,
                         std::string("D-Bus GetManagedObjects call failed: ") +
                             ctx->dbus.sysErr.message);
    dbus_error_free(&ctx->dbus.sysErr);
    dbus_message_unref(msg);
    return;
  }

  DBusMessageIter iter;
  dbus_message_iter_init(reply, &iter);

  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
    ctx->logger.LogError(
        TAG,
        "Unexpected argument type (Array Type) in GetManagedObjects reply");

    return;
  }

  DBusMessageIter arrayIter;
  dbus_message_iter_recurse(&iter, &arrayIter);

  while (dbus_message_iter_get_arg_type(&arrayIter) != DBUS_TYPE_INVALID) {

    DBusMessageIter dictEntryIter;
    dbus_message_iter_recurse(&arrayIter, &dictEntryIter);

    if (dbus_message_iter_get_arg_type(&dictEntryIter) !=
        DBUS_TYPE_OBJECT_PATH) {
      ctx->logger.LogError(TAG,
                           "Unexpected argument type (Object Path Type) in "
                           "GetManagedObjects reply");

      return;
    }

    char *name;
    dbus_message_iter_get_basic(&dictEntryIter, &name);

    dbus_message_iter_next(&dictEntryIter);

    DBusMessageIter inArrIter;
    dbus_message_iter_recurse(&dictEntryIter, &inArrIter);

    // Accessing 'net.connman.iwd.Device' properties
    dbus_message_iter_recurse(&inArrIter, &dictEntryIter);

    char *iface;
    dbus_message_iter_get_basic(&dictEntryIter, &iface);
    dbus_message_iter_next(&dictEntryIter);
    dbus_message_iter_recurse(&dictEntryIter, &inArrIter);

    if (HelperFunc::saferStrCmp(iface, "net.connman.iwd.Device")) {
      devPath = name;
      while (dbus_message_iter_get_arg_type(&inArrIter) ==
             DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter propIter;
        dbus_message_iter_recurse(&inArrIter, &propIter);

        char *propName;
        dbus_message_iter_get_basic(&propIter, &propName);

        DBusMessageIter valueIter;
        dbus_message_iter_next(&propIter);
        dbus_message_iter_recurse(&propIter, &valueIter);

        if (HelperFunc::saferStrNCmp(propName, "Powered", 7)) {
          dbus_bool_t powered;
          dbus_message_iter_get_basic(&valueIter, &powered);

          this->powered = powered;
        } else if (HelperFunc::saferStrNCmp(propName, "Address", 7)) {
          char *address;
          dbus_message_iter_get_basic(&valueIter, &address);

          devAddr = std::string(address);
        } else if (HelperFunc::saferStrNCmp(propName, "Name", 4)) {
          char *name;
          dbus_message_iter_get_basic(&valueIter, &name);

          devName = std::string(name);
        } else if (HelperFunc::saferStrNCmp(propName, "Adapter", 7)) {
          char *mode;
          dbus_message_iter_get_basic(&valueIter, &mode);

          devAdapter = std::string(mode);
        }

        dbus_message_iter_next(&inArrIter);
      }
    } else if (HelperFunc::saferStrCmp(iface, "net.connman.iwd.KnownNetwork")) {

      WifiStation dev;
      while (dbus_message_iter_get_arg_type(&inArrIter) ==
             DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter propIter;
        dbus_message_iter_recurse(&inArrIter, &propIter);

        char *propName;
        dbus_message_iter_get_basic(&propIter, &propName);

        DBusMessageIter valueIter;
        dbus_message_iter_next(&propIter);
        dbus_message_iter_recurse(&propIter, &valueIter);

        if (HelperFunc::saferStrCmp(propName, "Name")) {

          char *name;
          dbus_message_iter_get_basic(&valueIter, &name);
          dev.ssid = name;
        } else if (HelperFunc::saferStrCmp(propName, "Type")) {

          char *type;
          dbus_message_iter_get_basic(&valueIter, &type);
          dev.type = type;
        } else if (HelperFunc::saferStrCmp(propName, "AutoConnect")) {

          dbus_bool_t *autoConn;
          dbus_message_iter_get_basic(&valueIter, &autoConn);
          dev.autoConn = autoConn;
        }

        dbus_message_iter_next(&inArrIter);
      }

      dev.known = true;
      dev.rssi = -100;

      std::string nameStr(name);
      size_t pos = nameStr.rfind("/");
      if (pos != std::string_view::npos) {

        devices.insert({nameStr.substr(pos + 1), dev});
      }
    }

    dbus_message_iter_next(&arrayIter);
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);

  ctx->logger.LogInfo(TAG, "Successfully retrieved Device Path:" + devPath +
                               ", Address: " + devAddr + ", Name: " + devName +
                               ", Adapter: " + devAdapter);
  ctx->logger.LogInfo(TAG, "Known Networks Count: " +
                               std::to_string(devices.size()));
}

void WifiManager::Scan() {
  DBusMessage *msg = dbus_message_new_method_call(
      "net.connman.iwd", devPath.c_str(), "net.connman.iwd.Station", "Scan");
  if (!msg) {
    ctx->logger.LogError(TAG, "Failed to create D-Bus message for Scan");
    return;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, msg, -1, &ctx->dbus.sysErr);
  if (dbus_error_is_set(&ctx->dbus.sysErr) && !reply) {
    ctx->logger.LogError(TAG, std::string("D-Bus Scan call failed: ") +
                                  ctx->dbus.sysErr.message);
    dbus_error_free(&ctx->dbus.sysErr);
    dbus_message_unref(msg);
    return;
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);

  scanning = true;
}

int WifiManager::GetConnectedDevice() {
  DBusMessage *msg =
      dbus_message_new_method_call("net.connman.iwd", devPath.c_str(),
                                   "org.freedesktop.DBus.Properties", "Get");
  if (!msg) {
    ctx->logger.LogError(
        TAG, "Failed to create D-Bus message for Get Connected Device");
    return -1;
  }

  DBusMessageIter args;
  dbus_message_iter_init_append(msg, &args);

  const char *interfaceName = "net.connman.iwd.Station";
  const char *propertyName = "ConnectedNetwork";
  dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &interfaceName);
  dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &propertyName);

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, msg, -1, &ctx->dbus.sysErr);
  if (dbus_error_is_set(&ctx->dbus.sysErr) && !reply) {
    ctx->logger.LogError(
        TAG, std::string("D-Bus Get Connected Device call failed: ") +
                 ctx->dbus.sysErr.message);
    dbus_error_free(&ctx->dbus.sysErr);
    dbus_message_unref(msg);
    return -1;
  }

  DBusMessageIter replyIter, valueIter;
  dbus_message_iter_init(reply, &replyIter);
  dbus_message_iter_recurse(&replyIter, &valueIter);

  char *connDevPath;
  dbus_message_iter_get_basic(&valueIter, &connDevPath);
  connDev = connDevPath;

  size_t pos = connDev.rfind("/");
  if (pos != std::string_view::npos) {
    connDev = connDev.substr(pos + 1);
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);
  return 0;
}

void WifiManager::GetDevices() {
  DBusMessage *msg = dbus_message_new_method_call(
      "net.connman.iwd", devPath.c_str(), "net.connman.iwd.Station",
      "GetOrderedNetworks");
  if (!msg) {
    ctx->logger.LogError(
        TAG, "Failed to create D-Bus message for GetOrderedNetworks");
    return;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, msg, -1, &ctx->dbus.sysErr);
  if (dbus_error_is_set(&ctx->dbus.sysErr) && !reply) {
    ctx->logger.LogError(TAG,
                         std::string("D-Bus GetOrderedNetworks call failed: ") +
                             ctx->dbus.sysErr.message);
    dbus_error_free(&ctx->dbus.sysErr);
    dbus_message_unref(msg);
    return;
  }

  DBusMessageIter arrayIter, deviceIter;
  dbus_message_iter_init(reply, &arrayIter);
  dbus_message_iter_recurse(&arrayIter, &deviceIter);

  while (dbus_message_iter_get_arg_type(&deviceIter) != DBUS_TYPE_INVALID) {
    DBusMessageIter structIter;
    dbus_message_iter_recurse(&deviceIter, &structIter);

    std::string devicePath;
    WifiStation station;

    char *path;
    dbus_message_iter_get_basic(&structIter, &path);
    dbus_message_iter_next(&structIter);
    devicePath = path;

    size_t pos = devicePath.rfind("/");
    if (pos != std::string_view::npos) {
      devicePath = devicePath.substr(pos + 1);
    }

    dbus_int16_t rssi = 0;
    dbus_message_iter_get_basic(&structIter, &rssi);
    station.rssi = rssi / 100;

    auto it = devices.find(devicePath);
    if (it != devices.end()) {
      it->second.rssi = rssi / 100;
      dbus_message_iter_next(&deviceIter);
      continue;
    }

    if (!GetDeviceInfo(devicePath, station)) {
      station.known = false;
      devices.insert({devicePath, station});
    }
    dbus_message_iter_next(&deviceIter);
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);
}

int WifiManager::GetDeviceInfo(std::string dev, WifiStation &station) {

  DBusMessage *msg = dbus_message_new_method_call(
      "net.connman.iwd", (devPath + "/" + dev).c_str(),
      "org.freedesktop.DBus.Properties", "GetAll");
  if (!msg) {
    ctx->logger.LogError(TAG,
                         "Failed to create D-Bus message for Get Device Info");
    return -1;
  }

  DBusMessageIter args;
  dbus_message_iter_init_append(msg, &args);
  const char *interfaceName = "net.connman.iwd.Network";
  dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &interfaceName);

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, msg, -1, &ctx->dbus.sysErr);
  if (dbus_error_is_set(&ctx->dbus.sysErr) && !reply) {
    ctx->logger.LogError(TAG,
                         std::string("D-Bus Get Device Info call failed: ") +
                             ctx->dbus.sysErr.message);
    dbus_error_free(&ctx->dbus.sysErr);
    dbus_message_unref(msg);
    return -1;
  }

  DBusMessageIter arrayIter, structIter;
  dbus_message_iter_init(reply, &arrayIter);
  dbus_message_iter_recurse(&arrayIter, &structIter);

  while (dbus_message_iter_get_arg_type(&structIter) != DBUS_TYPE_INVALID) {
    DBusMessageIter propertyIter;
    dbus_message_iter_recurse(&structIter, &propertyIter);

    char *propName;
    dbus_message_iter_get_basic(&propertyIter, &propName);

    DBusMessageIter valueIter;
    dbus_message_iter_next(&propertyIter);
    dbus_message_iter_recurse(&propertyIter, &valueIter);

    if (HelperFunc::saferStrNCmp(propName, "Name", 4)) {
      char *name;
      dbus_message_iter_get_basic(&valueIter, &name);
      station.ssid = std::string(name);

    } else if (HelperFunc::saferStrNCmp(propName, "Connected", 9)) {
      dbus_bool_t connected;
      dbus_message_iter_get_basic(&valueIter, &connected);
      if (connected) {
        connDev = dev;
      }

    } else if (HelperFunc::saferStrNCmp(propName, "Type", 4)) {
      char *type;
      dbus_message_iter_get_basic(&valueIter, &type);
      station.type = std::string(type);
    }

    dbus_message_iter_next(&structIter);
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);
  return 0;
}

bool WifiManager::IsPowered() const { return powered; }
bool WifiManager::IsScanning() const { return scanning; }

void WifiManager::Connect(const std::string &networkPath) {
  std::string path = devPath + "/" + networkPath;

  DBusMessage *msg = dbus_message_new_method_call(
      "net.connman.iwd", path.c_str(), "net.connman.iwd.Network", "Connect");
  if (!msg) {
    ctx->logger.LogError(TAG, "Failed to create D-Bus message for Connect");
    return;
  }

  if (!dbus_connection_send(ctx->dbus.sysConn, msg, 0)) {
    ctx->logger.LogError(TAG, std::string("D-Bus Connect call failed: "));

    dbus_message_unref(msg);
    return;
  }

  dbus_message_unref(msg);
}

void WifiManager::Disconnect() {
  DBusMessage *msg =
      dbus_message_new_method_call("net.connman.iwd", devPath.c_str(),
                                   "net.connman.iwd.Station", "Disconnect");
  if (!msg) {
    ctx->logger.LogError(TAG, "Failed to create D-Bus message for Disconnect");
    return;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, msg, -1, &ctx->dbus.sysErr);
  if (dbus_error_is_set(&ctx->dbus.sysErr) && !reply) {
    ctx->logger.LogError(TAG, std::string("D-Bus Disconnect call failed: ") +
                                  ctx->dbus.sysErr.message);
    dbus_error_free(&ctx->dbus.sysErr);
    dbus_message_unref(msg);
    return;
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);
}

void WifiManager::Forget(const std::string &networkPath) {
  std::string path = "/net/connman/iwd/" + networkPath;

  DBusMessage *msg =
      dbus_message_new_method_call("net.connman.iwd", path.c_str(),
                                   "net.connman.iwd.KnownNetwork", "Forget");
  if (!msg) {
    ctx->logger.LogError(TAG, "Failed to create D-Bus message for Forget");
    return;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, msg, -1, &ctx->dbus.sysErr);
  if (dbus_error_is_set(&ctx->dbus.sysErr) && !reply) {
    ctx->logger.LogError(TAG, std::string("D-Bus Forget call failed: ") +
                                  ctx->dbus.sysErr.message);
    dbus_error_free(&ctx->dbus.sysErr);
    dbus_message_unref(msg);
    return;
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);
}

void WifiManager::SubmitPassphrase(const std::string &password) {
  if (authMsg == nullptr || authDev.empty()) {
    return;
  }

  DBusMessage *reply = dbus_message_new_method_return(authMsg);
  if (!reply) {
    ctx->logger.LogError(
        TAG, "Failed to create reply for Passphrase Method Response");
    return;
  }

  DBusMessageIter args;
  dbus_message_iter_init_append(reply, &args);

  const char *passphrase = password.c_str();
  dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &passphrase);
  dbus_message_iter_init_closed(&args);

  if (!dbus_connection_send(ctx->dbus.sysConn, reply, 0)) {
    ctx->logger.LogError(TAG, "Failed to send Passphrase Method Response");
    dbus_message_unref(reply);
    return;
  }

  dbus_connection_flush(ctx->dbus.sysConn);
  ctx->logger.LogInfo(TAG, "Submitted passphrase for device: " + authDev);
  
  dbus_message_unref(reply);
  dbus_message_unref(authMsg);
  authMsg = nullptr;
  authDev = "";
}

void WifiManager::addMatchRules() {
  ctx->logger.LogInfo(TAG, "Adding D-Bus Match Rules for WifiManager");

  dbus_bus_add_match(ctx->dbus.sysConn,
                     ("type='method_call',interface='net.connman.iwd.Agent',"
                      "member='RequestPassphrase',path='" +
                      agentPath + "'")
                         .c_str(),
                     &(ctx->dbus.sysErr));
  if (dbus_error_is_set(&(ctx->dbus.sysErr))) {
    std::string errMsg =
        "Failed to add filter for Method Call Member RequestPassphrase: ";
    errMsg += ctx->dbus.sysErr.message;
    ctx->logger.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.sysErr);
    return;
  }

  dbus_bus_add_match(ctx->dbus.sysConn,
                     ("type='method_call',interface='net.connman.iwd.Agent',"
                      "member='Cancel',path='" +
                      agentPath + "'")
                         .c_str(),
                     &ctx->dbus.sysErr);
  if (dbus_error_is_set(&ctx->dbus.sysErr)) {
    std::string errMsg =
        "Failed to add D-Bus match rule for InterfacesAdded signal: ";
    errMsg += ctx->dbus.sysErr.message;
    ctx->logger.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.sysErr);
    return;
  }

  dbus_bus_add_match(
      ctx->dbus.sysConn,
      "type='signal', interface='org.freedesktop.DBus.ObjectManager', "
      "member='InterfacesRemoved'",
      &(ctx->dbus.sysErr));
  if (dbus_error_is_set(&(ctx->dbus.sysErr))) {
    std::string errMsg =
        "Failed to add filter for Signal Member InterfaceRemoved: ";
    errMsg += ctx->dbus.sysErr.message;
    ctx->logger.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.sysErr);
    return;
  }

  dbus_bus_add_match(
      ctx->dbus.sysConn,
      "type='signal',interface='org.freedesktop.DBus.Properties',"
      "member='PropertiesChanged'",
      &ctx->dbus.sysErr);
  if (dbus_error_is_set(&ctx->dbus.sysErr)) {
    std::string errMsg =
        "Failed to add D-Bus match rule for PropertiesChanged signal: ";
    errMsg += ctx->dbus.sysErr.message;
    ctx->logger.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.sysErr);
    return;
  }

  ctx->logger.LogInfo(TAG,
                      "Successfully added D-Bus match rules for WifiManager");
}

void WifiManager::handleRequestPassphrase(DBusMessage *msg,
                                          DBusMessageIter &rootIter) {
  if (dbus_message_iter_get_arg_type(&rootIter) != DBUS_TYPE_OBJECT_PATH) {
    ctx->logger.LogError(TAG,
                         "Failed to parse RequestPassphrase signal: expected "
                         "first argument to be a objectpath");
    return;
  }

  char *path;
  dbus_message_iter_get_basic(&rootIter, &path);

  std::string ssid(path);
  size_t pos = ssid.rfind("/");
  if (pos != std::string_view::npos) {
    authMsg = msg;
    authDev = ssid.substr(pos + 1);
  }
}

void WifiManager::handleRequestCancel() {
  authDev = "";
  if (authMsg) {
    dbus_message_unref(authMsg);
    authMsg = nullptr;
  }
}

void WifiManager::handleInterfacesRemoved(DBusMessageIter &rootIter) {
  char *objPath;
  if (dbus_message_iter_get_arg_type(&rootIter) != DBUS_TYPE_OBJECT_PATH) {
    ctx->logger.LogError(TAG,
                         "Failed to parse InterfacesAdded signal: expected "
                         "first argument to be an object path");
    return;
  }

  dbus_message_iter_get_basic(&rootIter, &objPath);
  if (!HelperFunc::saferStrNCmp(objPath, "/net/connman/iwd", 16)) {
    return;
  }

  std::string objPathStr(objPath);
  size_t pos = objPathStr.rfind("/");
  if (pos != std::string_view::npos) {
    objPathStr = objPathStr.substr(pos + 1);
  }

  devices.erase(objPathStr);
  if (connDev == objPathStr) {
    connDev = "";
  }
}

void WifiManager::handlePropertiesChanged(DBusMessage *msg,
                                          DBusMessageIter &rootIter) {

  char *iface;
  if (dbus_message_iter_get_arg_type(&rootIter) != DBUS_TYPE_STRING) {
    ctx->logger.LogError(TAG,
                         "Failed to parse PropertiesChanged signal: expected "
                         "first argument to be a string (interface name)");
    return;
  }

  dbus_message_iter_get_basic(&rootIter, &iface);
  dbus_message_iter_next(&rootIter);

  std::string_view ifaceStr(iface);
  if (ifaceStr == "net.connman.iwd.Station") {

    DBusMessageIter arrIter;
    dbus_message_iter_recurse(&rootIter, &arrIter);

    while (dbus_message_iter_get_arg_type(&arrIter) != DBUS_TYPE_INVALID) {
      DBusMessageIter dictIter;
      dbus_message_iter_recurse(&arrIter, &dictIter);

      char *propName;
      if (dbus_message_iter_get_arg_type(&dictIter) != DBUS_TYPE_STRING) {
        ctx->logger.LogError(TAG, "Failed to parse PropertiesChanged signal: "
                                  "expected property name to be a string");
        return;
      }

      dbus_message_iter_get_basic(&dictIter, &propName);
      if (HelperFunc::saferStrNCmp(propName, "Scanning", 8)) {
        dbus_message_iter_next(&dictIter);
        dbus_bool_t scanning;
        if (dbus_message_iter_get_arg_type(&dictIter) != DBUS_TYPE_VARIANT) {
          ctx->logger.LogError(
              TAG, "Failed to parse PropertiesChanged signal: expected "
                   "Connected property value to be a boolean variant");
          return;
        }

        DBusMessageIter variantIter;
        dbus_message_iter_recurse(&dictIter, &variantIter);
        if (dbus_message_iter_get_arg_type(&variantIter) != DBUS_TYPE_BOOLEAN) {
          ctx->logger.LogError(
              TAG, "Failed to parse PropertiesChanged signal: expected "
                   "Connected property value to be a boolean");
          return;
        }

        dbus_message_iter_get_basic(&variantIter, &scanning);
        this->scanning = scanning;
      }

      dbus_message_iter_next(&arrIter);
    }
  } else if (ifaceStr == "net.connman.iwd.Network") {
    DBusMessageIter arrIter;
    dbus_message_iter_recurse(&rootIter, &arrIter);

    while (dbus_message_iter_get_arg_type(&arrIter) != DBUS_TYPE_INVALID) {
      DBusMessageIter dictIter;
      dbus_message_iter_recurse(&arrIter, &dictIter);

      char *propName;
      if (dbus_message_iter_get_arg_type(&dictIter) != DBUS_TYPE_STRING) {
        ctx->logger.LogError(TAG, "Failed to parse PropertiesChanged signal: "
                                  "expected property name to be a string");
        return;
      }

      dbus_message_iter_get_basic(&dictIter, &propName);
      if (HelperFunc::saferStrNCmp(propName, "Connected", 9)) {
        dbus_message_iter_next(&dictIter);
        dbus_bool_t connected;
        if (dbus_message_iter_get_arg_type(&dictIter) != DBUS_TYPE_VARIANT) {
          ctx->logger.LogError(
              TAG, "Failed to parse PropertiesChanged signal: expected "
                   "Connected property value to be a boolean variant");
          return;
        }

        DBusMessageIter variantIter;
        dbus_message_iter_recurse(&dictIter, &variantIter);
        if (dbus_message_iter_get_arg_type(&variantIter) != DBUS_TYPE_BOOLEAN) {
          ctx->logger.LogError(
              TAG, "Failed to parse PropertiesChanged signal: expected "
                   "Connected property value to be a boolean");
          return;
        }

        dbus_message_iter_get_basic(&variantIter, &connected);
        if (connected) {
          const char *path = dbus_message_get_path(msg);
          connDev = path;
        } else {
          connDev = "";
        }

        size_t pos = connDev.rfind("/");
        if (pos != std::string_view::npos) {
          connDev = connDev.substr(pos + 1);
        }
      }

      dbus_message_iter_next(&arrIter);
    }
  }
}

WifiManager::~WifiManager() { RegisterAgent(false); }
