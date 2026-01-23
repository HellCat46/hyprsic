#pragma once
#include "../database/db_manager.hpp"
#include "../logging/manager.hpp"
#include "dbus/dbus.h"

class DbusSystem {
public:
  DBusConnection *sysConn;
  DBusError sysErr;

  DBusConnection *ssnConn;
  DBusError ssnErr;

  DbusSystem();
  ~DbusSystem();
  
  void DictToInt64(DBusMessageIter *iter, uint64_t& outValue);
  void DictToString(DBusMessageIter *iter, std::string& outValue);
};

class AppContext {
public:
  DbusSystem dbus;
  DBManager dbManager;
  LoggingManager logging;
  AppContext();
};
