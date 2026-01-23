#include "context.hpp"
#include "cstring"
#include "dbus/dbus.h"
#include "iostream"

AppContext::AppContext() : dbus(), logging(true), dbManager(&logging) {}

DbusSystem::DbusSystem() {
  dbus_error_init(&sysErr);
  dbus_error_init(&ssnErr);
  sysConn = dbus_bus_get(DBUS_BUS_SYSTEM, &sysErr);
  if (!sysConn) {
    std::cerr << "[Error] Failed to Connect With the DBUS System: "
              << sysErr.name
              << "\n[DBUS Error Message - Context] : " << sysErr.message
              << std::endl;
    return;
  }

  ssnConn = dbus_bus_get(DBUS_BUS_SESSION, &ssnErr);
  if (!ssnConn) {
    std::cerr << "[Error] Failed to Connect With the DBUS Session: "
              << ssnErr.name
              << "\n[DBUS Error Message - Context] : " << ssnErr.message
              << std::endl;
  }
  
  dbus_threads_init_default();
}

DbusSystem::~DbusSystem() {
  dbus_error_free(&ssnErr);
  dbus_error_free(&sysErr);

  dbus_connection_flush(sysConn);
  dbus_connection_close(sysConn);

  dbus_connection_flush(ssnConn);
  dbus_connection_close(ssnConn);
}

void DbusSystem::DictToInt64(DBusMessageIter *iter, uint64_t *outValue) {
  DBusMessageIter variantIter;
  dbus_message_iter_recurse(iter, &variantIter);

  if (dbus_message_iter_get_arg_type(&variantIter) == DBUS_TYPE_INT64) {
    dbus_message_iter_get_basic(&variantIter, outValue);
  }
}

void DbusSystem::DictToString(DBusMessageIter *iter, std::string *outValue) {
  DBusMessageIter variantIter;
  dbus_message_iter_recurse(iter, &variantIter);

  if (dbus_message_iter_get_arg_type(&variantIter) == DBUS_TYPE_STRING ||
      dbus_message_iter_get_arg_type(&variantIter) == DBUS_TYPE_OBJECT_PATH) {
    char *strValue;
    dbus_message_iter_get_basic(&variantIter, &strValue);
    *outValue = std::string(strValue);
  }
}
