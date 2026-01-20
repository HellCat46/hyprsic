#pragma once

#include "../../app/context.hpp"
#include "dbus/dbus.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include <map>
#include <string>
#include <thread>

struct MenuActionItem {
  int index;
  std::string label;
  bool visible;
  bool enabled;
  bool isSeparator;
};

struct StatusApp {
  std::string menu_path, status;
  GdkPixbuf* pixmap;
  std::map<int, std::string> menuActions;
};

class StatusNotifierManager {
  AppContext *ctx;
  std::string SNWXML;
  std::thread ssnThread;
  std::map<std::string, StatusApp> registeredItems;

public:
  StatusNotifierManager(AppContext *appCtx);

  void captureStatusNotifier();

  // DBus Introspectable and Properties Handler
  void handleIntrospectCall(DBusMessage *msg);
  void handleGetAllPropertiesCall(DBusMessage *msg);
  void handleGetPropertyCall(DBusMessage *msg);
  void handleNameOwnerChangedSignal(DBusMessage *msg);
  
  // Functions to get Additional Info about Registered Items
  void getItemInfo(const std::string &itemService, StatusApp &outApp);
  void getMenuActions(const std::string &itemService, StatusApp &outApp);

  // DBus Message Handler for Status Notifier Watcher Interface
  void handleRegisterStatusNotifierHost(DBusMessage *msg);
  void handleRegisterStatusNotifierItem(DBusMessage *msg);
};
