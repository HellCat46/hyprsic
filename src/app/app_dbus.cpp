#include "app.hpp"
#include "dbus/dbus.h"
#include "utils/helper_func.hpp"

#define TAG "Application_DBUS"

void Application::captureSessionDBus() {
  notifManager.setupDBus();
  snManager.setupDBus();

  DBusMessage *msg;
  while (1) {
    if (!dbus_connection_read_write_dispatch(ctx.dbus.ssnConn, 100)) {
      ctx.logger.LogError(
          TAG, "Connection Closed while Waiting for Notification Messages");
      return;
    }

    msg = dbus_connection_pop_message(ctx.dbus.ssnConn);
    if (!msg) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    const char *interface = dbus_message_get_interface(msg);
    const char *path = dbus_message_get_path(msg);
    const char *member = dbus_message_get_member(msg);

    if (HelperFunc::saferStrCmp(interface, "org.freedesktop.Notifications")) {
      notifManager.handleDbusMessage(msg, NotificationWindow::showNotification);
    } else if (HelperFunc::saferStrCmp(interface, "org.freedesktop.DBus") &&
               HelperFunc::saferStrCmp(member, "NameOwnerChanged") &&
               HelperFunc::saferStrCmp(path, "/org/freedesktop/DBus")) {

      DBusMessageIter args;
      dbus_message_iter_init(msg, &args);

      const char *name, *oldOwner, *newOwner;
      dbus_message_iter_get_basic(&args, &name);
      dbus_message_iter_next(&args);
      dbus_message_iter_get_basic(&args, &oldOwner);
      dbus_message_iter_next(&args);
      dbus_message_iter_get_basic(&args, &newOwner);

      if (HelperFunc::saferStrNCmp(name, "org.mpris.MediaPlayer2", 22)) {
        if (std::strlen(newOwner) != 0) {
          mprisManager.addPlayer(name);
        } else {
          mprisManager.removePlayer(name);
        }
      }

      snManager.handleNameOwnerChangedSignal(msg, name, newOwner);
    }
    {
      snManager.handleDbusMessage(msg);
    }

    dbus_message_unref(msg);
  }
}

void Application::captureSystemDBus() {
  btManager.addMatchRules();

  DBusMessage *msg;
  while (true) {
    // Blocks the thread until new message received
    if (!dbus_connection_read_write(ctx.dbus.sysConn, 0)) {
      ctx.logger.LogError(
          TAG, "Connection Closed while Waiting for Signal Messages");
      return;
    }

    msg = dbus_connection_pop_message(ctx.dbus.sysConn);
    if (!msg) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    DBusMessageIter rootIter;
    dbus_message_iter_init(msg, &rootIter);

    if (dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager",
                               "InterfacesAdded")) {
      char *path;
      dbus_message_iter_get_basic(&rootIter, &path);

      if (HelperFunc::saferStrNCmp(path, "/org/bluez", 10)) {
        ctx.logger.LogDebug(TAG, "Received InterfacesAdded Signal for Bluez");
        btManager.handleInterfacesAdded(rootIter);
      }

      dbus_message_unref(msg);
    } else if (dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager",
                                      "InterfacesRemoved")) {
      char *path;
      dbus_message_iter_get_basic(&rootIter, &path);

      if (HelperFunc::saferStrNCmp(path, "/org/bluez", 10)) {
        btManager.handleInterfacesRemoved(rootIter);
      } else {
        wifiManager.handleInterfacesRemoved(rootIter);
      }

      dbus_message_unref(msg);
    } else if (dbus_message_is_signal(msg, "org.freedesktop.DBus.Properties",
                                      "PropertiesChanged")) {
      const char *path = dbus_message_get_path(msg);

      if (HelperFunc::saferStrNCmp(path, "/org/bluez", 10)) {

        btManager.handlePropertiesChanged(msg, rootIter);
      } else if (HelperFunc::saferStrNCmp(path, "/net/connman/iwd", 16)) {

        wifiManager.handlePropertiesChanged(msg, rootIter);
      }

      dbus_message_unref(msg);
    } else if (dbus_message_is_method_call(msg, "net.connman.iwd.Agent",
                                           "RequestPassphrase")) {
      wifiManager.handleRequestPassphrase(msg, rootIter);
    } else if (dbus_message_is_method_call(msg, "net.connman.iwd.Agent",
                                           "Cancel")) {
      wifiManager.handleRequestCancel();
    } else {
      ctx.logger.LogInfo(TAG, "Received Unknown Signal on System Bus");
    }
  }
}
