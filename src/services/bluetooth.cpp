#include "bluetooth.hpp"
#include "cstring"
#include "iostream"
#include "thread"
#include "unordered_map"
#include "../utils/dbus_utils.hpp"

BluetoothManager::BluetoothManager(AppContext *context) {
  ctx = context;
  discover = false;
  signalThread = std::thread(&BluetoothManager::monitorChanges, this);
}

int BluetoothManager::switchDiscovery() {
  if (discover) {

    std::cout << "[INFO] Turning off Bluetooth Discovery." << std::endl;
    discover = false;
  } else {
    DBusMessage *msg = dbus_message_new_method_call(
        "org.bluez", "/org/bluez/hci0", "org.bluez.Adapter1", "StartDiscovery");
    if (!msg) {
      std::cerr << "[Error] Failed to create a message. " << ctx->dbus.err.message
                << std::endl;
      return 1;
    }

    DBusMessage *reply = dbus_connection_send_with_reply_and_block(
        ctx->dbus.conn, msg, -1, &(ctx->dbus.err));
    if (!reply) {
      std::cerr << "[Error] Failed to get a reply. " << ctx->dbus.err.message
                << std::endl;
      return 1;
    }

    std::cout << "[INFO] Turning on Bluetooth Discovery." << std::endl;
    this->discover = true;
  }
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
                 "InterfaceAdded"
              << std::endl;
    return;
  }

  dbus_bus_add_match(
      ctx->dbus.conn,
      "type='signal', interface='org.freedesktop.DBus.ObjectManager', "
      "member='InterfacesRemoved'",
      &(ctx->dbus.err));
  if (dbus_error_is_set(&(ctx->dbus.err))) {
    std::cerr << "[Critical Error] Failed to add filter for Signal Member "
                 "InterfaceRemoved"
              << std::endl;
    return;
  }

  dbus_bus_add_match(
      ctx->dbus.conn,
      "type='signal', interface='org.freedesktop.DBus.Properties', "
      "member='PropertiesChanged'",
      &(ctx->dbus.err));
  if (dbus_error_is_set(&(ctx->dbus.err))) {
    std::cerr << "[Critical Error] Failed to add filter for Signal Member "
                 "PropestiesChanged"
              << std::endl;
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

      Device dev{"", "", "", -110, DeviceType::Unknown, false, false};
      if (setDeviceProps(dev, propsIter) != 0) {
        continue;
      }

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
      if (std::strncmp(path, "/org/bluez", 10) != 0) {
        continue;
      }

      auto dev = devices.find(path);
      if (dev == devices.end()) {
        std::cerr << "[Error] Unable to find Device in Device List. Skipping."
                  << std::endl;
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

      if (setDeviceProps(dev->second, entIter) != 0) {
        devices.erase(dev);
        std::cout << "[Info] Removed Device from Device List as It is now "
                     "connected. Total Devices: "
                  << devices.size() << std::endl;
        continue;
      }

      std::cout << "[Info] Updated Device Properties. Total Devices: "
                << devices.size() << std::endl;
    } else {
      std::cerr << "[Info] Received Unknown Signal" << std::endl;
    }

    dbus_message_unref(msg);
  }
  return;
}

int BluetoothManager::setDeviceProps(Device &dev, DBusMessageIter &propsIter) {
  std::string props[] = {"Adapter", "Address", "Connected",
                         "Paired",  "RSSI",    "Trusted"};
  DBusMessageIter values[6];
  DbusUtils::getProperties(propsIter, props, 6, values);

  // Check whether Device is already connected
  if (props[2][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[2]) == DBUS_TYPE_BOOLEAN) {
    dbus_bool_t value;
    dbus_message_iter_get_basic(&values[2], &value);
    if (value)
      return 1;
  }

  // For Adapter Path
  if (props[0][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[0]) == DBUS_TYPE_OBJECT_PATH) {
    char *value;
    dbus_message_iter_get_basic(&values[0], &value);
    dev.adapter = value;
  }

  // For Address
  if (props[1][0] == ' ' &&
      dbus_message_iter_get_arg_type(&values[1]) == DBUS_TYPE_STRING) {
    char *value;
    dbus_message_iter_get_basic(&values[1], &value);
    dev.address = value;
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
  return 0;
}

int BluetoothManager::getDeviceList() {
  if (devices.size() == 0)
    return 1;

  for (auto &[_, value] : devices) {
    std::cout << "Device: " << std::endl;
    std::cout << "\t" << value.name << std::endl;
    std::cout << "\t" << value.address << std::endl;
    std::cout << "\t" << value.adapter << std::endl;
    std::cout << "\t" << value.paired << std::endl;
    std::cout << "\t" << value.trusted << std::endl;
    std::cout << "\t" << value.rssi << std::endl;
  }

  std::cout << "[Info] Device Count: " << devices.size() << std::endl;
  return 0;
}
