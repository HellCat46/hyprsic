#pragma once
#include "../database/db_manager.hpp"
#include "dbus/dbus.h"

class DbusSystem {
public:
  DBusConnection *sysConn;
  DBusError sysErr;

  DBusConnection *ssnConn;
  DBusError ssnErr;
  
  DbusSystem();
  ~DbusSystem();
};

class AppContext {
public:
  DbusSystem dbus;
  DBManager dbManager;
  AppContext();
};
