#pragma once

#include "../../app/context.hpp"
#include "dbus/dbus.h"
#include <thread>

class StatusNotifierManager {
  AppContext *ctx;
  std::string SNWXML;
  std::thread ssnThread;

public:
  StatusNotifierManager(AppContext *appCtx);

  void captureStatusNotifier();

  // DBus Introspectable and Properties Handler
  void handleIntrospectCall(DBusMessage *msg);
  void handleGetAllPropertiesCall(DBusMessage *msg);

  // DBus Message Handler for Status Notifier Watcher Interface
  static void registerStatusNotifierHost(DBusConnection *conn, DBusMessage *msg,
                                         void *user_data);
  static void registerStatusNotifierItem(DBusConnection *conn, DBusMessage *msg,
                                         void *user_data);
};
