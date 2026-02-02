#include "manager.hpp"
#include "../../utils/helper_func.hpp"
#include "dbus/dbus-protocol.h"
#include "dbus/dbus.h"
#include <cstring>
#include <string>

#define TAG "WifiManager"

WifiManager::WifiManager(AppContext *appCtx) : ctx(appCtx) {

  DBusMessage *msg = dbus_message_new_method_call(
      "net.connman.iwd", "/", "org.freedesktop.DBus.ObjectManager",
      "GetManagedObjects");
  if (!msg) {
    ctx->logging.LogError(TAG, "Failed to create D-Bus message for ListNames");
    return;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, msg, -1, &ctx->dbus.sysErr);
  if (dbus_error_is_set(&ctx->dbus.sysErr) && !reply) {
    ctx->logging.LogError(TAG,
                          std::string("D-Bus GetManagedObjects call failed: ") +
                              ctx->dbus.sysErr.message);
    dbus_error_free(&ctx->dbus.sysErr);
    dbus_message_unref(msg);
    return;
  }

  DBusMessageIter iter;
  dbus_message_iter_init(reply, &iter);

  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
    ctx->logging.LogError(
        TAG,
        "Unexpected argument type (Array Type) in GetManagedObjects reply");

    return;
  }

  DBusMessageIter arrayIter;
  dbus_message_iter_recurse(&iter, &arrayIter);

  // Skip Root Object Path (/)
  dbus_message_iter_next(&arrayIter);

  // Skip Dbus Interface Path (/net/connman/iwd)
  dbus_message_iter_next(&arrayIter);

  // Skip Physical Device Object Path
  dbus_message_iter_next(&arrayIter);

  DBusMessageIter dictEntryIter;
  dbus_message_iter_recurse(&arrayIter, &dictEntryIter);

  if (dbus_message_iter_get_arg_type(&dictEntryIter) != DBUS_TYPE_OBJECT_PATH) {
    ctx->logging.LogError(TAG, "Unexpected argument type (Object Path Type) in "
                               "GetManagedObjects reply");

    return;
  }

  char *name;
  dbus_message_iter_get_basic(&dictEntryIter, &name);
  devPath = std::string(name);

  dbus_message_iter_next(&dictEntryIter);
  dbus_message_iter_recurse(&dictEntryIter, &arrayIter);

  // Accessing 'net.connman.iwd.Device' properties
  dbus_message_iter_recurse(&arrayIter, &dictEntryIter);
  dbus_message_iter_next(&dictEntryIter);
  dbus_message_iter_recurse(&dictEntryIter, &arrayIter);

  while (dbus_message_iter_get_arg_type(&arrayIter) == DBUS_TYPE_DICT_ENTRY) {
    DBusMessageIter propertyIter;
    dbus_message_iter_recurse(&arrayIter, &propertyIter);

    char *propName;
    dbus_message_iter_get_basic(&propertyIter, &propName);

    DBusMessageIter valueIter;
    dbus_message_iter_next(&propertyIter);
    dbus_message_iter_recurse(&propertyIter, &valueIter);

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

    dbus_message_iter_next(&arrayIter);
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);

  GetConnectedDevice();
  GetDevices();
}

int WifiManager::GetConnectedDevice() {
  DBusMessage *msg =
      dbus_message_new_method_call("net.connman.iwd", "/net/connman/iwd/0/4",
                                   "org.freedesktop.DBus.Properties", "Get");
  if (!msg) {
    ctx->logging.LogError(
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
    ctx->logging.LogError(
        TAG, std::string("D-Bus Get Connected Device call failed: ") +
                 ctx->dbus.sysErr.message);
    dbus_error_free(&ctx->dbus.sysErr);
    dbus_message_unref(msg);
    return -1;
  }

  DBusMessageIter replyIter, valueIter;
  dbus_message_iter_init(reply, &replyIter);
  dbus_message_iter_recurse(&replyIter, &valueIter);

  char *connectedDevPath;
  dbus_message_iter_get_basic(&valueIter, &connectedDevPath);
  connectedDev = std::string(connectedDevPath);

  dbus_message_unref(msg);
  dbus_message_unref(reply);
  return 0;
}

void WifiManager::GetDevices() {
  DBusMessage *msg = dbus_message_new_method_call(
      "net.connman.iwd", "/net/connman/iwd/0/4", "net.connman.iwd.Station",
      "GetOrderedNetworks");
  if (!msg) {
    ctx->logging.LogError(
        TAG, "Failed to create D-Bus message for GetOrderedNetworks");
    return;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, msg, -1, &ctx->dbus.sysErr);
  if (dbus_error_is_set(&ctx->dbus.sysErr) && !reply) {
    ctx->logging.LogError(
        TAG, std::string("D-Bus GetOrderedNetworks call failed: ") +
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
    devicePath = std::string(path);

    dbus_int16_t rssi = 0;
    dbus_message_iter_get_basic(&structIter, &rssi);
    station.rssi = rssi;

    auto it = devices.find(devicePath);
    if (it != devices.end()) {
      devices[devicePath].rssi = rssi;
      dbus_message_iter_next(&deviceIter);
      continue;
    }

    if (!GetDeviceInfo(devicePath, station)) {
      devices.insert({devicePath, station});
    }
    dbus_message_iter_next(&deviceIter);
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);
}

int WifiManager::GetDeviceInfo(std::string devPath, WifiStation &station) {
  DBusMessage *msg =
      dbus_message_new_method_call("net.connman.iwd", devPath.c_str(),
                                   "org.freedesktop.DBus.Properties", "GetAll");
  if (!msg) {
    ctx->logging.LogError(TAG,
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
    ctx->logging.LogError(TAG,
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
      station.connected = connected;

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
