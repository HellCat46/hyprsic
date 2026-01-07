#include "context.hpp"
#include "cstring"
#include "dbus/dbus.h"
#include "iostream"

AppContext::AppContext() : dbus(), dbManager() {}

DbusSystem::DbusSystem() {
  dbus_error_init(&sysErr);
  dbus_error_init(&ssnErr);
  sysConn = dbus_bus_get(DBUS_BUS_SYSTEM, &sysErr);
  if (!sysConn) {
    std::cerr << "[Error] Failed to Connect With the DBUS System: "
              << sysErr.name << "\n[DBUS Error Message] : " << sysErr.message
              << std::endl;
    return;
  }

  ssnConn = dbus_bus_get(DBUS_BUS_SESSION, &ssnErr);
  if (!ssnConn) {
    std::cerr << "[Error] Failed to Connect With the DBUS Session: "
              << ssnErr.name << "\n[DBUS Error Message] : " << ssnErr.message
              << std::endl;
  }

  std::cout
      << "[Info] Successfully created a connection to Dbus System Session."
      << std::endl;
}

DbusSystem::~DbusSystem() {
  dbus_error_free(&ssnErr);
  dbus_error_free(&sysErr);

  dbus_connection_flush(sysConn);
  dbus_connection_close(sysConn);

  dbus_connection_flush(ssnConn);
  dbus_connection_close(ssnConn);
}
