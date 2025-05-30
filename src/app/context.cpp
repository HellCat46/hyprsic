#include "context.hpp"
#include "cstring"
#include "dbus/dbus.h"
#include "iostream"

AppContext::AppContext() : dbus() {}

DbusSystem::DbusSystem() {
  dbus_error_init(&err);
  conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if (!conn) {
    std::cerr << "[Error] Failed to Connect With the DBUS Session: " << err.name
              << "\n[DBUS Error Message] : " << err.message << std::endl;
    return;
  }

  std::cout
      << "[Info] Successfully created a connection to Dbus System Session."
      << std::endl;
}
DbusSystem::~DbusSystem() {
  dbus_error_free(&err);
  dbus_connection_flush(conn);
  dbus_connection_close(conn);
}
