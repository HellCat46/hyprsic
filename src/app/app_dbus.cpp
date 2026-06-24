#include "header/app.hpp"
#include <sdbus-c++/Error.h>
#include <sdbus-c++/Message.h>

#define TAG "Application_DBUS"

void Application::captureSessionDBus() {
  ctx.dbus.ssnConn->addMatch(
      "type='signal', sender='org.freedesktop.DBus', "
      "interface='org.freedesktop.DBus', member='NameOwnerChanged', "
      "path='/org/freedesktop/DBus'",
      [this](sdbus::Message msg) {
        std::string name, oldOwner, newOwner;
        msg >> name >> oldOwner >> newOwner;

        mprisManager.handlePlayerChangesDbus(name, newOwner);
        snManager.handleNameOwnerChangedSignalDbus(name, newOwner);
      });

  ctx.dbus.ssnConn->enterEventLoopAsync();
}

void Application::captureSystemDBus() {
  wifiManager.addMatchRulesDbus();

  ctx.dbus.sysConn->addMatch(
      "type='signal', interface='org.freedesktop.DBus.ObjectManager', "
      "member='InterfacesAdded'",
      [this](sdbus::Message msg) {
        std::string path;
        msg >> path;

        if (path.compare(0, 10, "/org/bluez") == 0) {
          btManager.handleInterfacesAddedDbus(msg);
        }
      });

  ctx.dbus.sysConn->addMatch(
      "type='signal', interface='org.freedesktop.DBus.ObjectManager', "
      "member='InterfacesRemoved'",
      [this](sdbus::Message msg) {
        std::string path;
        msg >> path;

        if (path.compare(0, 10, "/org/bluez") == 0) {
          btManager.handleInterfacesRemovedDbus(msg);
        }
      });

  ctx.dbus.sysConn->addMatch(
      "type='signal', interface='org.freedesktop.DBus.Properties', "
      "member='PropertiesChanged'",
      [this](sdbus::Message msg) {
        std::string path = msg.getPath();

        if (path.compare(0, 10, "/org/bluez") == 0) {
          btManager.handlePropertiesChangedDbus(msg);
        } else if (path.compare(0, 16, "/net/connman/iwd") == 0) {
          wifiManager.handlePropertiesChangedDbus(msg);
        }
      });

  ctx.dbus.sysConn->enterEventLoopAsync();
}
