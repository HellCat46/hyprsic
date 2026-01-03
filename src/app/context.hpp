#pragma once
#include "dbus/dbus.h"

class DbusSystem {
public:
  DBusConnection *conn;
  DBusError err;

  DbusSystem();
  ~DbusSystem();
};

class AppContext {
public:
  DbusSystem dbus;
  AppContext();
};
