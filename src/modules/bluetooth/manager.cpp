#include "manager.hpp"
#include "../../utils/dbus_utils.hpp"
#include "../../utils/helper_func.hpp"
#include "cstring"
#include "dbus/dbus-protocol.h"
#include "dbus/dbus.h"
#include "thread"
#include "unordered_map"
#include <string>

#define TAG "BluetoothManager"

BluetoothManager::BluetoothManager(AppContext *ctx) : ctx(ctx) {
  discovering = false;
  power = true;

  devListMsg = dbus_message_new_method_call(
      "org.bluez", "/", "org.freedesktop.DBus.ObjectManager",
      "GetManagedObjects");
  if (!devListMsg) {
    ctx->logger.LogError(TAG, "Failed to create a message.");
    return;
  }
}

int BluetoothManager::setup() {
  int res = getPropertyVal("Powered");
  if (res >= 0) {
    std::string msg = "Initial Bluetooth Power State: ";
    msg += (res ? "ON" : "OFF");
    ctx->logger.LogInfo(TAG, msg);
    power = res;
  } else {
    ctx->logger.LogWarning(TAG, "Unable to get initial Bluetooth Power State. "
                                "Setting to ON by default.");
    power = true;
  }

  res = getPropertyVal("Discovering");
  if (res >= 0) {
    std::string msg = "Initial Bluetooth Discovery State: ";
    msg += (res ? "ON" : "OFF");
    ctx->logger.LogInfo(TAG, msg);
    discovering = res;
  } else {
    ctx->logger.LogWarning(TAG,
                           "Unable to get initial Bluetooth Discovery State. "
                           "Setting to OFF by default.");
    discovering = false;
  }

  getDeviceList();

  signalThread = std::thread(&BluetoothManager::monitorChanges, this);
  return 0;
}

int BluetoothManager::switchPower(bool on) {
  DBusMessage *msg = dbus_message_new_method_call(
      "org.bluez", "/org/bluez/hci0", "org.freedesktop.DBus.Properties", "Set");
  if (!msg) {
    std::string errMsg = "Failed to create a message. ";
    errMsg += ctx->dbus.sysErr.message;
    ctx->logger.LogError(TAG, errMsg);
    return 1;
  }

  const char *iface = "org.bluez.Adapter1";
  const char *prop = "Powered";
  dbus_bool_t value = on ? TRUE : FALSE;
  DBusMessageIter args, subargs;

  dbus_message_iter_init_append(msg, &args);
  dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &iface);
  dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop);

  dbus_message_iter_open_container(&args, DBUS_TYPE_VARIANT,
                                   DBUS_TYPE_BOOLEAN_AS_STRING, &subargs);
  dbus_message_iter_append_basic(&subargs, DBUS_TYPE_BOOLEAN, &value);
  dbus_message_iter_close_container(&args, &subargs);

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, msg, -1, &(ctx->dbus.sysErr));
  if (!reply && dbus_error_is_set(&(ctx->dbus.sysErr))) {
    std::string errMsg = "Failed to get a reply. ";
    errMsg += ctx->dbus.sysErr.message;
    ctx->logger.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.sysErr);
    return 1;
  }

  std::string logMsg = "Turned ";
  logMsg += (on ? "ON" : "OFF");
  logMsg += " Bluetooth Power.";
  ctx->logger.LogInfo(TAG, logMsg);

  dbus_message_unref(msg);
  dbus_message_ref(reply);
  this->power = on;
  return 0;
}

int BluetoothManager::switchDiscovery(bool on) {
  DBusMessage *msg = dbus_message_new_method_call(
      "org.bluez", "/org/bluez/hci0", "org.bluez.Adapter1",
      on ? "StartDiscovery" : "StopDiscovery");

  if (!msg) {
    ctx->logger.LogError(TAG, "Failed to create a message.");
    return 1;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, msg, -1, &(ctx->dbus.sysErr));
  if (!reply && dbus_error_is_set(&(ctx->dbus.sysErr))) {
    std::string errMsg = "Failed to get a reply. ";
    errMsg += ctx->dbus.sysErr.message;
    ctx->logger.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.sysErr);
    return 1;
  }

  dbus_message_unref(msg);
  dbus_message_ref(reply);

  std::string logMsg = "Turning ";
  logMsg += (on ? "ON" : "OFF");
  logMsg += " Bluetooth Discovery.";
  ctx->logger.LogInfo(TAG, logMsg);
  this->discovering = on;

  return 0;
}

void BluetoothManager::monitorChanges() {
  if (addMatchRules())
    return;

  DBusMessage *msg;
  while (true) {
    // Blocks the thread until new message received
    if (!dbus_connection_read_write(ctx->dbus.sysConn, 0)) {
      ctx->logger.LogError(
          TAG, "Connection Closed while Waiting for Signal Messages");
      return;
    }

    msg = dbus_connection_pop_message(ctx->dbus.sysConn);
    if (!msg) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    DBusMessageIter rootIter;
    dbus_message_iter_init(msg, &rootIter);

    if (dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager",
                               "InterfacesAdded")) {

      ctx->logger.LogDebug(TAG, "Received InterfacesAdded Signal");
      handleInterfacesAdded(msg, rootIter);
    } else if (dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager",
                                      "InterfacesRemoved")) {

      ctx->logger.LogDebug(TAG, "Received InterfacesRemoved Signal");
      handleInterfacesRemoved(msg, rootIter);
    } else if (dbus_message_is_signal(msg, "org.freedesktop.DBus.Properties",
                                      "PropertiesChanged")) {

      // ctx->logger.LogDebug(TAG, "Received PropertiesChanged Signal");
      handlePropertiesChanged(msg, rootIter);
    } else {

      ctx->logger.LogInfo(TAG, "Received Unknown Signal");
    }

    dbus_message_unref(msg);
  }
  return;
}

int BluetoothManager::addMatchRules() {
  ctx->logger.LogInfo(TAG, "Adding filter to Bluez Signals");

  // Adding Filters to Signals before starting listening to them
  dbus_bus_add_match(
      ctx->dbus.sysConn,
      "type='signal', interface='org.freedesktop.DBus.ObjectManager', "
      "member='InterfacesAdded'",
      &(ctx->dbus.sysErr));
  if (dbus_error_is_set(&(ctx->dbus.sysErr))) {
    std::string errMsg =
        "Failed to add filter for Signal Member InterfaceAdded: ";
    errMsg += ctx->dbus.sysErr.message;
    ctx->logger.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.sysErr);
    return 1;
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
    return 1;
  }

  dbus_bus_add_match(
      ctx->dbus.sysConn,
      "type='signal', interface='org.freedesktop.DBus.Properties', "
      "member='PropertiesChanged'",
      &(ctx->dbus.sysErr));
  if (dbus_error_is_set(&(ctx->dbus.sysErr))) {
    std::string errMsg =
        "Failed to add filter for Signal Member PropertiesChanged: ";
    errMsg += ctx->dbus.sysErr.message;
    ctx->logger.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.sysErr);
    return 1;
  }
  ctx->logger.LogInfo(TAG, "Successfully Added Filters to Bluez Dbus Signals. "
                           "Started listening to events now.");

  return 0;
}

void BluetoothManager::handleInterfacesAdded(DBusMessage *msg,
                                             DBusMessageIter &rootIter) {

  if (dbus_message_iter_get_arg_type(&rootIter) != DBUS_TYPE_OBJECT_PATH) {
    ctx->logger.LogError(TAG, "Unable to parse InterfacesAdded Reply. Unknown "
                              "Format (The First Entry is not Object Path.)");
    return;
  }

  // Getting Object Path
  char *path;
  dbus_message_iter_get_basic(&rootIter, &path);

  // Moving to next entry after objectPath entry
  DBusMessageIter entIter, devIter, propsIter;
  dbus_message_iter_next(&rootIter);
  dbus_message_iter_recurse(&rootIter, &entIter);
  if (dbus_message_iter_get_arg_type(&entIter) != DBUS_TYPE_DICT_ENTRY) {
    ctx->logger.LogError(TAG,
                         "Unable to parse InterfacesAdded Reply. Unknown "
                         "Format (The Second Entry is not an Dict Entry.)");
    return;
  }

  // Looking for Device1 Entry
  while (dbus_message_iter_get_arg_type(&entIter) == DBUS_TYPE_DICT_ENTRY) {
    char *objPath;

    dbus_message_iter_recurse(&entIter, &devIter);
    dbus_message_iter_get_basic(&devIter, &objPath);
    if (HelperFunc::saferStrNCmp(objPath, "org.bluez.Device1", 17)) {
      break;
    }
    dbus_message_iter_next(&entIter);
  }

  if (dbus_message_iter_get_arg_type(&entIter) != DBUS_TYPE_DICT_ENTRY) {
    ctx->logger.LogError(
        TAG, "Unable to find Device1 Entry in InterfacesAdded Reply.");
    return;
  }

  // Getting Properties
  dbus_message_iter_next(&devIter);
  dbus_message_iter_recurse(&devIter, &propsIter);

  Device dev{"", "", path, "", -110, false, false, false, false, -1};
  setDeviceProps(dev, propsIter);

  devices.insert({path, dev});
  std::string logMsg = "Added Device to Device List. Total Devices: ";
  logMsg += std::to_string(devices.size());
  ctx->logger.LogInfo(TAG, logMsg);
}

void BluetoothManager::handleInterfacesRemoved(DBusMessage *msg,
                                               DBusMessageIter &rootIter) {
  if (dbus_message_iter_get_arg_type(&rootIter) != DBUS_TYPE_OBJECT_PATH) {
    ctx->logger.LogError(TAG,
                         "Unable to parse InterfacesRemoved Reply. Unknown "
                         "Format (The First Entry is not Object Path.)");
    return;
  }

  char *path;
  dbus_message_iter_get_basic(&rootIter, &path);
  if (devices.find(path) != devices.end()) {
    devices.erase(path);
    std::string logMsg = "Removed Device from Device List. Total Devices: ";
    logMsg += std::to_string(devices.size());
    ctx->logger.LogInfo(TAG, logMsg);
  } else {
    ctx->logger.LogWarning(TAG,
                           "Unable to find Device in Device List. Skipping.");
  }
}

void BluetoothManager::handlePropertiesChanged(DBusMessage *msg,
                                               DBusMessageIter &rootIter) {
  const char *path = dbus_message_get_path(msg);
  if (!HelperFunc::saferStrNCmp(path, "/org/bluez", 10) ||
      std::strlen(path) > 37)
    return;

  // Extract Device Address from Object Path
  path = std::strstr(path, "dev_");
  if (!path || std::strlen(path) < 21) {
    ctx->logger.LogWarning(TAG,
                           "Unable to Extract Device Address from Object Path. "
                           "Unknown Format. Skipping.");
    return;
  }

  std::string addr = path + 4; // Move past "dev_"
  for (int i = 0; i < addr.length(); i++) {
    if (addr[i] == '_')
      addr[i] = ':';
  }

  auto dev = devices.find(addr);
  if (dev == devices.end()) {
    std::string errMsg = "Unable to find Device in Device List. Skipping. ";
    errMsg += addr;
    ctx->logger.LogError(TAG, errMsg);
    return;
  }

  DBusMessageIter entIter;
  if (dbus_message_iter_get_arg_type(&rootIter) != DBUS_TYPE_STRING) {
    ctx->logger.LogError(TAG,
                         "Unable to parse PropertiesChanged Reply. Unknown "
                         "Format (The First Entry is not String.)");
    return;
  }

  char *iface;
  dbus_message_iter_get_basic(&rootIter, &iface);
  if (!HelperFunc::saferStrCmp(iface, "org.bluez.Device1")) {
    return;
  }

  dbus_message_iter_next(&rootIter);
  while (dbus_message_iter_get_arg_type(&rootIter) == DBUS_TYPE_ARRAY) {
    dbus_message_iter_recurse(&rootIter, &entIter);

    if (dbus_message_iter_get_arg_type(&entIter) == DBUS_TYPE_DICT_ENTRY) {
      break;
    }
    ctx->logger.LogInfo(
        TAG, std::to_string(dbus_message_iter_get_arg_type(&entIter)));

    dbus_message_iter_next(&rootIter);
  }
  if (dbus_message_iter_get_arg_type(&entIter) != DBUS_TYPE_DICT_ENTRY) {
    ctx->logger.LogError(TAG,
                         "Unable to parse PropertiesChanged Reply. Unknown "
                         "Format (No Dict Entry for Property Found.)");
    return;
  }

  Device newDevData{"", "", "", "", -110, false, false, false, false, -1};
  unsigned char flags = setDeviceProps(newDevData, entIter);

  if (flags == 0) {
    ctx->logger.LogWarning(TAG, "No Property Updated. Skipping.");
    return;
  }

  if (flags & DevicePropFlags::NAME)
    dev->second.name = newDevData.name;

  if (flags & DevicePropFlags::ADDRESS)
    dev->second.addr = newDevData.addr;

  if (flags & DevicePropFlags::CONNECTED) {
    dev->second.connected = newDevData.connected;

    if (newDevData.connected) {
      ctx->showUpdateWindow(UpdateModule::BLUETOOTH, dev->second.deviceType == "audio-headset"? "headset_mic" : "connected",
                            "Connected to Device: " + dev->second.name);
    } else {
      ctx->showUpdateWindow(UpdateModule::BLUETOOTH, "base",
                            "Disconnected from Device: " + dev->second.name);
    }
  }

  if (flags & DevicePropFlags::PAIRED)
    dev->second.paired = newDevData.paired;

  if (flags & DevicePropFlags::RSSI)
    dev->second.rssi = newDevData.rssi;

  if (flags & DevicePropFlags::TRUSTED)
    dev->second.trusted = newDevData.trusted;

  if (flags & DevicePropFlags::DEVICE_TYPE)
    dev->second.deviceType = newDevData.deviceType;

  std::string updateMsg = "Updated Device Properties. Total Devices: " +
                          std::to_string(devices.size()) +
                          ". Properties Update Flag: " + std::to_string(flags);
  ctx->logger.LogInfo(TAG, updateMsg);
}

int BluetoothManager::getDeviceList() {
  devices.clear();

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, devListMsg, -1, &(ctx->dbus.sysErr));
  if (!reply && dbus_error_is_set(&(ctx->dbus.sysErr))) {
    std::string errMsg = "Failed to get a reply. ";
    errMsg += ctx->dbus.sysErr.message;
    ctx->logger.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.sysErr);
    return 1;
  }

  DBusMessageIter rootIter, entIter;
  dbus_message_iter_init(reply, &rootIter);

  if (dbus_message_iter_get_arg_type(&rootIter) != DBUS_TYPE_ARRAY) {
    ctx->logger.LogError(TAG, "No Device Connected");
    return -1;
  }

  dbus_message_iter_recurse(&rootIter, &entIter);

  while (dbus_message_iter_get_arg_type(&entIter) == DBUS_TYPE_DICT_ENTRY) {
    DBusMessageIter entry, ifaceIter;
    char *objPath;

    dbus_message_iter_recurse(&entIter, &entry);
    dbus_message_iter_get_basic(&entry, &objPath);

    // Skip every entry (including endpoint and transport) except the actual
    // device
    if (!(std::strlen(objPath) == 37 &&
          HelperFunc::saferStrNCmp(objPath, "/org/bluez/hci0/dev", 19))) {
      dbus_message_iter_next(&entIter);
      continue;
    }

    dbus_message_iter_next(&entry);
    dbus_message_iter_recurse(&entry, &ifaceIter);

    Device deviceInfo{"",    "",    objPath, "",    -110,
                      false, false, false,   false, -1};

    while (dbus_message_iter_get_arg_type(&ifaceIter) == DBUS_TYPE_DICT_ENTRY) {
      DBusMessageIter ifaceEntry, propsIter;
      char *ifaceName;

      dbus_message_iter_recurse(&ifaceIter, &ifaceEntry);
      dbus_message_iter_get_basic(&ifaceEntry, &ifaceName);

      dbus_message_iter_next(&ifaceEntry);
      dbus_message_iter_recurse(&ifaceEntry, &propsIter);

      if (HelperFunc::saferStrCmp(ifaceName, "org.bluez.Device1")) {
        setDeviceProps(deviceInfo, propsIter);
      } else if (HelperFunc::saferStrCmp(ifaceName, "org.bluez.Battery1")) {
        std::string properties[] = {"Percentage"};
        DBusMessageIter values[1];
        DbusUtils::getProperties(propsIter, properties, 1, values);

        if (dbus_message_iter_get_arg_type(&values[0]) == DBUS_TYPE_BYTE) {
          int value;
          dbus_message_iter_get_basic(&values[0], &value);
          deviceInfo.batteryPer = value;
        }
      } else if (HelperFunc::saferStrCmp(ifaceName,
                                         "org.bluez.MediaControl1")) {
        std::string properties[] = {"Connected"};
        DBusMessageIter values[1];
        DbusUtils::getProperties(propsIter, properties, 1, values);

        // For Connected
        if (dbus_message_iter_get_arg_type(&values[0]) == DBUS_TYPE_BOOLEAN) {
          dbus_bool_t value;
          dbus_message_iter_get_basic(&values[0], &value);
          deviceInfo.mediaConnected = value;
        }
      }

      dbus_message_iter_next(&ifaceIter);
    }

    devices.insert({deviceInfo.addr, deviceInfo});
    dbus_message_iter_next(&entIter);
  }

  return 0;
}

int BluetoothManager::getPropertyVal(const char *prop) {
  DBusMessage *msg = dbus_message_new_method_call(
      "org.bluez", "/org/bluez/hci0", "org.freedesktop.DBus.Properties", "Get");
  if (!msg) {
    ctx->logger.LogError(TAG, "Failed to create a message.");
    return -1;
  }

  const char *iface = "org.bluez.Adapter1";

  DBusMessageIter args;
  dbus_message_iter_init_append(msg, &args);
  dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &iface);
  dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop);

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, msg, -1, &(ctx->dbus.sysErr));
  if (!reply && dbus_error_is_set(&(ctx->dbus.sysErr))) {
    std::string errMsg = "Failed to get a reply. ";
    errMsg += ctx->dbus.sysErr.message;
    ctx->logger.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.sysErr);
    return -1;
  }
  dbus_message_unref(msg);

  DBusMessageIter rootIter, variant;

  if (!dbus_message_iter_init(reply, &rootIter)) {
    ctx->logger.LogError(TAG, "Reply has no arguments!");
    return -1;
  }

  if (dbus_message_iter_get_arg_type(&rootIter) != DBUS_TYPE_VARIANT) {
    ctx->logger.LogError(TAG, "Argument is not a variant!");
    return -1;
  }

  dbus_message_iter_recurse(&rootIter, &variant);

  if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_BOOLEAN) {
    dbus_bool_t value;
    dbus_message_iter_get_basic(&variant, &value);
    return value;
  } else {
    ctx->logger.LogError(TAG, "Argument is not a boolean!");
    return -1;
  }

  return -1;
}

int BluetoothManager::connectDevice(bool state, const char *devPath) {
  std::string logMsg = state ? "Connecting" : "Disconnecting";
  logMsg += " to Device: ";
  logMsg += devPath;
  ctx->logger.LogInfo(TAG, logMsg);

  DBusMessage *msg =
      dbus_message_new_method_call("org.bluez", devPath, "org.bluez.Device1",
                                   state ? "Connect" : "Disconnect");
  if (!msg) {
    ctx->logger.LogError(TAG, "Failed to create a message.");
    return 1;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, msg, -1, &(ctx->dbus.sysErr));
  if (!reply && dbus_error_is_set(&(ctx->dbus.sysErr))) {
    std::string errMsg = "Failed to get a reply. ";
    errMsg += ctx->dbus.sysErr.message;
    ctx->logger.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.sysErr);
    return 1;
  }

  dbus_message_unref(msg);
  dbus_message_ref(reply);

  std::string successMsg = state ? "Connected" : "Disconnected";
  successMsg += " to Device: ";
  successMsg += devPath;
  ctx->logger.LogInfo(TAG, successMsg);

  return 0;
}

unsigned char BluetoothManager::setDeviceProps(Device &dev,
                                               DBusMessageIter &propsIter) {
  std::string props[] = {"Name", "Address", "Connected", "Paired",
                         "RSSI", "Trusted", "Icon"};
  unsigned char propFlags = 0;
  DBusMessageIter values[7];
  DbusUtils::getProperties(propsIter, props, 7, values);

  // For Connected
  if (props[2][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[2]) == DBUS_TYPE_BOOLEAN) {
    dbus_bool_t value;
    dbus_message_iter_get_basic(&values[2], &value);
    dev.connected = value;
    propFlags |= DevicePropFlags::CONNECTED;
  }

  // For Device Name (Alias)
  if (props[0][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[0]) == DBUS_TYPE_STRING) {
    char *value;
    dbus_message_iter_get_basic(&values[0], &value);
    dev.name = value;

    propFlags |= DevicePropFlags::NAME;
  }

  // For Address
  if (props[1][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[1]) == DBUS_TYPE_STRING) {
    char *value;
    dbus_message_iter_get_basic(&values[1], &value);
    dev.addr = value;

    propFlags |= DevicePropFlags::ADDRESS;
  }

  // For Paired
  if (props[3][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[3]) == DBUS_TYPE_BOOLEAN) {
    dbus_bool_t value;
    dbus_message_iter_get_basic(&values[3], &value);
    dev.paired = value;

    propFlags |= DevicePropFlags::PAIRED;
  }

  // For RSSI
  if (props[4][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[4]) == DBUS_TYPE_INT16) {
    dbus_int16_t value;
    dbus_message_iter_get_basic(&values[4], &value);
    dev.rssi = value;

    propFlags |= DevicePropFlags::RSSI;
  }

  // For Trusted
  if (props[5][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[5]) == DBUS_TYPE_BOOLEAN) {
    dbus_bool_t value;
    dbus_message_iter_get_basic(&values[5], &value);
    dev.trusted = value;

    propFlags |= DevicePropFlags::TRUSTED;
  }

  // For Device Icon/Type
  if (props[6][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[6]) == DBUS_TYPE_STRING) {
    char *value;
    dbus_message_iter_get_basic(&values[6], &value);
    dev.deviceType = value;

    propFlags |= DevicePropFlags::DEVICE_TYPE;
    
  }

  return propFlags;
}
