#include "bluetooth.hpp"
#include "../utils/dbus_utils.hpp"
#include "cstring"
#include "dbus/dbus-protocol.h"
#include "dbus/dbus.h"
#include "iostream"
#include "thread"
#include "unordered_map"

BluetoothManager::BluetoothManager(AppContext *context) {
  ctx = context;
  discovering = false;
  power = true;

  devListMsg = dbus_message_new_method_call(
      "org.bluez", "/", "org.freedesktop.DBus.ObjectManager",
      "GetManagedObjects");
  if (!devListMsg) {
    std::cerr << "Failed to create a message." << std::endl;
    return;
  }

  int res = getPropertyVal("Powered");
  if (res >= 0) {
    std::cout << "[Info] Initial Bluetooth Power State: "
              << (res ? "ON" : "OFF") << std::endl;
    power = res;
  } else {
    std::cerr << "[Warning] Unable to get initial Bluetooth Power State. "
                 "Setting to ON by default."
              << std::endl;
    power = true;
  }

  res = getPropertyVal("Discovering");
  if (res >= 0) {
    std::cout << "[Info] Initial Bluetooth Discovery State: "
              << (res ? "ON" : "OFF") << std::endl;
    discovering = res;
  } else {
    std::cerr << "[Warning] Unable to get initial Bluetooth Discovery State. "
                 "Setting to OFF by default."
              << std::endl;
    discovering = false;
  }

  getDeviceList();

  // signalThread = std::thread(&BluetoothManager::monitorChanges, this);
}

int BluetoothManager::switchPower(bool on) {
  DBusMessage *msg = dbus_message_new_method_call(
      "org.bluez", "/org/bluez/hci0", "org.freedesktop.DBus.Properties", "Set");
  if (!msg) {
    std::cerr << "[Error] Failed to create a message. " << ctx->dbus.err.message
              << std::endl;
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
      ctx->dbus.conn, msg, -1, &(ctx->dbus.err));
  if (!reply && dbus_error_is_set(&(ctx->dbus.err))) {
    std::cerr << "[Error] Failed to get a reply. " << ctx->dbus.err.message
              << std::endl;
    dbus_error_free(&ctx->dbus.err);
    return 1;
  }

  std::cout << "[INFO] Turned " << (on ? "ON" : "OFF") << " Bluetooth Power."
            << std::endl;

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
    std::cerr << "[Error] Failed to create a message. " << std::endl;
    return 1;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.conn, msg, -1, &(ctx->dbus.err));
  if (!reply && dbus_error_is_set(&(ctx->dbus.err))) {
    std::cerr << "[Error] Failed to get a reply. " << ctx->dbus.err.message
              << std::endl;
    dbus_error_free(&ctx->dbus.err);
    return 1;
  }

  dbus_message_unref(msg);
  dbus_message_ref(reply);
  std::cout << "[INFO] Turning " << (on ? "ON" : "OFF")
            << " Bluetooth Discovery." << std::endl;
  this->discovering = on;

  return 0;
}

void BluetoothManager::monitorChanges() {
  std::cout << "[Info] Adding filter to Bluez Signals" << std::endl;

  // Adding Filters to Signals before starting listening to them
  dbus_bus_add_match(
      ctx->dbus.conn,
      "type='signal', interface='org.freedesktop.DBus.ObjectManager', "
      "member='InterfacesAdded'",
      &(ctx->dbus.err));
  if (dbus_error_is_set(&(ctx->dbus.err))) {
    std::cerr << "[Critical Error] Failed to add filter for Signal Member "
                 "InterfaceAdded: "
              << ctx->dbus.err.message << std::endl;
    dbus_error_free(&ctx->dbus.err);
    return;
  }

  dbus_bus_add_match(
      ctx->dbus.conn,
      "type='signal', interface='org.freedesktop.DBus.ObjectManager', "
      "member='InterfacesRemoved'",
      &(ctx->dbus.err));
  if (dbus_error_is_set(&(ctx->dbus.err))) {
    std::cerr << "[Critical Error] Failed to add filter for Signal Member "
                 "InterfaceRemoved: "
              << ctx->dbus.err.message << std::endl;
    dbus_error_free(&ctx->dbus.err);
    return;
  }

  dbus_bus_add_match(
      ctx->dbus.conn,
      "type='signal', interface='org.freedesktop.DBus.Properties', "
      "member='PropertiesChanged'",
      &(ctx->dbus.err));
  if (dbus_error_is_set(&(ctx->dbus.err))) {
    std::cerr << "[Critical Error] Failed to add filter for Signal Member "
                 "PropertiesChanged: "
              << ctx->dbus.err.message << std::endl;
    dbus_error_free(&ctx->dbus.err);
    return;
  }
  std::cout << "[Info] Successfully Added Filters to Bluez Dbus Signals. "
               "Started listening to events now."
            << std::endl;

  DBusMessage *msg;
  while (true) {
    // Blocks the thread until new message received
    if (!dbus_connection_read_write(ctx->dbus.conn, 0)) {
      std::cerr << "[Critical Error] Connection Closed while Waiting for "
                   "Signal Messages"
                << std::endl;
      return;
    }

    msg = dbus_connection_pop_message(ctx->dbus.conn);
    if (!msg) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }
    std::cout << "\n[Info] Received Signal Message" << std::endl;

    DBusMessageIter rootIter;
    dbus_message_iter_init(msg, &rootIter);

    if (dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager",
                               "InterfacesAdded")) {
      std::cout << "[Info] Received InterfacesAdded Signal" << std::endl;

      if (dbus_message_iter_get_arg_type(&rootIter) != DBUS_TYPE_OBJECT_PATH) {
        std::cerr << "Unable to parse InterfacesAdded Reply. Unknown Format "
                     "(The First Entry is not Object Path.)"
                  << std::endl;
        continue;
      }

      // Getting Object Path
      char *path;
      dbus_message_iter_get_basic(&rootIter, &path);

      // Moving to next entry after objectPath entry
      DBusMessageIter entIter, devIter, propsIter;
      dbus_message_iter_next(&rootIter);
      dbus_message_iter_recurse(&rootIter, &entIter);
      if (dbus_message_iter_get_arg_type(&entIter) != DBUS_TYPE_DICT_ENTRY) {
        std::cerr << "[Error] Unable to parse InterfacesAdded Reply. Unknown "
                     "Format (The Second Entry is not an Dict Entry.)"
                  << std::endl;
        continue;
      }

      // Looking for Device1 Entry
      while (dbus_message_iter_get_arg_type(&entIter) == DBUS_TYPE_DICT_ENTRY) {
        char *objPath;

        dbus_message_iter_recurse(&entIter, &devIter);
        dbus_message_iter_get_basic(&devIter, &objPath);
        if (std::strncmp(objPath, "org.bluez.Device1", 17) == 0) {
          break;
        }
        dbus_message_iter_next(&entIter);
      }

      if (dbus_message_iter_get_arg_type(&entIter) != DBUS_TYPE_DICT_ENTRY) {
        std::cerr
            << "[Error] Unable to find Device1 Entry in InterfacesAdded Reply."
            << std::endl;
        continue;
      }

      // Getting Properties
      dbus_message_iter_next(&devIter);
      dbus_message_iter_recurse(&devIter, &propsIter);

      Device dev{"", "", path, "", -110, false, false, false, false, -1};
      setDeviceProps(dev, propsIter);

      devices.insert({path, dev});
      std::cout << "[Info] Added Device to Device List. Total Devices: "
                << devices.size() << std::endl;
    } else if (dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager",
                                      "InterfacesRemoved")) {
      std::cout << "[Info] Received InterfacesRemoved Signal" << std::endl;
      if (dbus_message_iter_get_arg_type(&rootIter) != DBUS_TYPE_OBJECT_PATH) {
        std::cerr << "[Error] Unable to parse InterfacesRemoved Reply. Unknown "
                     "Format (The First Entry is not Object Path.)"
                  << std::endl;
        continue;
      }

      char *path;
      dbus_message_iter_get_basic(&rootIter, &path);
      if (devices.find(path) != devices.end()) {
        devices.erase(path);
        std::cout << "[Info] Removed Device from Device List. Total Devices: "
                  << devices.size() << std::endl;
      } else {
        std::cerr << "[Warning] Unable to find Device in Device List. Skipping."
                  << std::endl;
      }
    } else if (dbus_message_is_signal(msg, "org.freedesktop.DBus.Properties",
                                      "PropertiesChanged")) {
      std::cout << "[Info] Received PropertiesChanged Signal" << std::endl;
      const char *path = dbus_message_get_path(msg);
      if (std::strncmp(path, "/org/bluez", 10) != 0)
        continue;

      std::cout << "[Debug] PropertiesChanged Signal Path: " << path
                << std::endl;

      char addr[18];
      for (int i = 0; i < 17; i++) {
        if (path[20 + i] == '_')
          addr[i] = ':';
        else
          addr[i] = path[20 + i];
      }
      addr[17] = '\0';
      // std::cout<<"[Debug] Parsed Address from Path: " << addr << std::endl;

      auto dev = devices.find(addr);
      if (dev == devices.end()) {
        std::cerr << "[Error] Unable to find Device in Device List. Skipping."
                  << addr << std::endl;
        continue;
      }

      DBusMessageIter entIter;
      if (dbus_message_iter_get_arg_type(&rootIter) != DBUS_TYPE_STRING) {
        std::cerr << "[Error] Unable to parse PropertiesChanged Reply. Unknown "
                     "Format (The First Entry is not String.)"
                  << std::endl;
        continue;
      }

      char *iface;
      dbus_message_iter_get_basic(&rootIter, &iface);
      if (std::strcmp(iface, "org.bluez.Device1") != 0) {
        continue;
      }

      dbus_message_iter_next(&rootIter);
      while (dbus_message_iter_get_arg_type(&rootIter) ==
             DBUS_TYPE_ARRAY_AS_STRING[0]) {
        dbus_message_iter_recurse(&rootIter, &entIter);

        if (dbus_message_iter_get_arg_type(&entIter) == DBUS_TYPE_DICT_ENTRY) {
          break;
        }
        std::cout << dbus_message_iter_get_arg_type(&entIter) << std::endl;

        dbus_message_iter_next(&rootIter);
      }
      if (dbus_message_iter_get_arg_type(&entIter) != DBUS_TYPE_DICT_ENTRY) {
        std::cout << "[Error] Unable to parse PropertiesChanged Reply. Unknown "
                     "Format (No Dict Entry for Property Found.)"
                  << std::endl;
        continue;
      }

      Device newDevData{"", "", "", "", -110, false, false, false, false, -1};
      setDeviceProps(newDevData, entIter);
      devices.insert({dev->first, newDevData});

      std::cout << "[Info] Updated Device Properties. Total Devices: "
                << devices.size() << std::endl;

      getDeviceList();
    } else {
      std::cerr << "[Info] Received Unknown Signal" << std::endl;
    }

    dbus_message_unref(msg);
  }
  return;
}

int BluetoothManager::getDeviceList() {
  devices.clear();
  std::cout << "[Info] Fetching Bluetooth Device List..." << std::endl;

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.conn, devListMsg, -1, &(ctx->dbus.err));
  if (!reply && dbus_error_is_set(&(ctx->dbus.err))) {
    std::cerr << "Failed to get a reply. " << ctx->dbus.err.message
              << std::endl;
    dbus_error_free(&ctx->dbus.err);
    return 1;
  }

  DBusMessageIter rootIter, entIter;
  dbus_message_iter_init(reply, &rootIter);

  if (dbus_message_iter_get_arg_type(&rootIter) != DBUS_TYPE_ARRAY) {
    std::cerr << "No Device Connected" << std::endl;
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
          std::strncmp(objPath, "/org/bluez/hci0/dev", 19) == 0)) {
      dbus_message_iter_next(&entIter);
      continue;
    }

    dbus_message_iter_next(&entry);
    dbus_message_iter_recurse(&entry, &ifaceIter);

    Device deviceInfo{"",    "",    objPath, "",    -110,
                      false, false, false, false, -1};

    while (dbus_message_iter_get_arg_type(&ifaceIter) == DBUS_TYPE_DICT_ENTRY) {
      DBusMessageIter ifaceEntry, propsIter;
      char *ifaceName;

      dbus_message_iter_recurse(&ifaceIter, &ifaceEntry);
      dbus_message_iter_get_basic(&ifaceEntry, &ifaceName);

      dbus_message_iter_next(&ifaceEntry);
      dbus_message_iter_recurse(&ifaceEntry, &propsIter);

      if (std::strcmp(ifaceName, "org.bluez.Device1") == 0) {
        setDeviceProps(deviceInfo, propsIter);
      } else if (std::strcmp(ifaceName, "org.bluez.Battery1") == 0) {
        std::string properties[] = {"Percentage"};
        DBusMessageIter values[1];
        DbusUtils::getProperties(propsIter, properties, 1, values);

        if (dbus_message_iter_get_arg_type(&values[0]) == DBUS_TYPE_BYTE) {
          int value;
          dbus_message_iter_get_basic(&values[0], &value);
          deviceInfo.batteryPer = value;
        }
      } else if (std::strcmp(ifaceName, "org.bluez.MediaControl1") == 0) {
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

  std::cout << "[Info] Fetched Bluetooth Device List. Total Devices: "
            << devices.size() << std::endl;

  return 0;
}

int BluetoothManager::getPropertyVal(const char *prop) {
  DBusMessage *msg = dbus_message_new_method_call(
      "org.bluez", "/org/bluez/hci0", "org.freedesktop.DBus.Properties", "Get");
  if (!msg) {
    std::cerr << "[Error] Failed to create a message. " << std::endl;
    return -1;
  }

  const char *iface = "org.bluez.Adapter1";

  DBusMessageIter args;
  dbus_message_iter_init_append(msg, &args);
  dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &iface);
  dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop);

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.conn, msg, -1, &(ctx->dbus.err));
  if (!reply && dbus_error_is_set(&(ctx->dbus.err))) {
    std::cerr << "[Error] Failed to get a reply. " << ctx->dbus.err.message
              << std::endl;
    dbus_error_free(&ctx->dbus.err);
    return -1;
  }
  dbus_message_unref(msg);

  DBusMessageIter rootIter, variant;

  if (!dbus_message_iter_init(reply, &rootIter)) {
    std::cout << "[Error] Reply has no arguments!" << std::endl;
    return -1;
  }

  if (dbus_message_iter_get_arg_type(&rootIter) != DBUS_TYPE_VARIANT) {
    std::cout << "[Error] Argument is not a variant!" << std::endl;
    return -1;
  }

  dbus_message_iter_recurse(&rootIter, &variant);

  if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_BOOLEAN) {
    dbus_bool_t value;
    dbus_message_iter_get_basic(&variant, &value);
    return value;
  } else {
    std::cout << "[Error] Argument is not a boolean!" << std::endl;
    return -1;
  }

  return -1;
}

int BluetoothManager::connectDevice(GtkWidget *widget, gpointer user_data) {
  FuncArgs *args = static_cast<FuncArgs *>(user_data);
  std::cout << "[Info] " << (args->state ? "Connecting" : "Disconnecting")
            << " to Device: " << args->devIfacePath << std::endl;

  DBusMessage *msg = dbus_message_new_method_call(
      "org.bluez", args->devIfacePath, "org.bluez.Device1",
      args->state ? "Connect" : "Disconnect");
  if (!msg) {
    std::cerr << "[Error] Failed to create a message. " << std::endl
              << std::endl;
    return 1;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      args->ctx->dbus.conn, msg, -1, &(args->ctx->dbus.err));
  if (!reply && dbus_error_is_set(&(args->ctx->dbus.err))) {
    std::cerr << "[Error] Failed to get a reply. "
              << args->ctx->dbus.err.message << std::endl;
    dbus_error_free(&args->ctx->dbus.err);
    return 1;
  }

  dbus_message_unref(msg);
  dbus_message_ref(reply);
  std::cout << "[INFO]" << (args->state ? "Connected" : "Disconnected")
            << " to Device: " << args->devIfacePath << std::endl;

  return 0;
}

void BluetoothManager::setDeviceProps(Device &dev, DBusMessageIter &propsIter) {
  std::string props[] = {"Name", "Address", "Connected", "Paired",
                         "RSSI", "Trusted", "Icon"};
  DBusMessageIter values[7];
  DbusUtils::getProperties(propsIter, props, 7, values);

  // For Connected 
  if (props[2][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[2]) == DBUS_TYPE_BOOLEAN) {
    dbus_bool_t value;
    dbus_message_iter_get_basic(&values[2], &value);
    dev.connected = value;
  }

  // For Device Name (Alias)
  if (props[0][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[0]) == DBUS_TYPE_STRING) {
    char *value;
    dbus_message_iter_get_basic(&values[0], &value);
    dev.name = value;
  }

  // For Address
  if (props[1][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[1]) == DBUS_TYPE_STRING) {
    char *value;
    dbus_message_iter_get_basic(&values[1], &value);
    dev.addr = value;
  }

  // For Paired
  if (props[3][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[3]) == DBUS_TYPE_BOOLEAN) {
    dbus_bool_t value;
    dbus_message_iter_get_basic(&values[3], &value);
    dev.paired = value;
  }

  // For RSSI
  if (props[4][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[4]) == DBUS_TYPE_INT16) {
    dbus_int16_t value;
    dbus_message_iter_get_basic(&values[4], &value);
    dev.rssi = value;
  }

  // For Trusted
  if (props[5][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[5]) == DBUS_TYPE_BOOLEAN) {
    dbus_bool_t value;
    dbus_message_iter_get_basic(&values[5], &value);
    dev.trusted = value;
  }

  // For Device Icon/Type
  if (props[6][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[1]) == DBUS_TYPE_STRING) {
    char *value;
    dbus_message_iter_get_basic(&values[1], &value);
    dev.deviceType = value;
  }
}

bool BluetoothManager::printDevicesInfo() {
  if (devices.size() == 0)
    return false;

  for (auto [_, device] : devices) {
    std::cout << "Device: " << std::endl;

    std::cout << "\t" << device.name << std::endl;
    std::cout << "\t" << device.addr << std::endl;
    std::cout << "\t" << device.path << std::endl;
    std::cout << "\t" << device.rssi << std::endl;
    std::cout << "\t" << device.paired << std::endl;
    std::cout << "\t" << device.trusted << std::endl;
    std::cout << "\t" << device.deviceType << std::endl;
    std::cout << "\t" << device.connected << std::endl;
    std::cout << "\t" << device.mediaConnected << std::endl;
    std::cout << "\t" << device.batteryPer << std::endl;
  }
  return true;
}
