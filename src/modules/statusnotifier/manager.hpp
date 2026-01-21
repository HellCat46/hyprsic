#pragma once

#include "../../app/context.hpp"
#include "dbus/dbus.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "gtk/gtk.h"
#include <functional>
#include <map>
#include <string>
#include <sys/types.h>
#include <thread>
#include <vector>

struct MenuActionItem {
  int index;
  std::string label;
  bool visible;
  bool enabled;
  bool isSeparator;
};

struct StatusApp {
  std::string menu_path, status;
  GdkPixbuf *pixmap;
  std::map<int, MenuActionItem> menuActions;
};


struct SNIApp {
    GtkWidget* icon;
    GtkWidget* popOver;
    GtkWidget* parentBox;
};

struct RemoveCallback {
  std::function<void(std::string servicePath, std::map<std::string, SNIApp>* sniApps, GtkWidget* sniBox)> callback;
  GtkWidget* widget;
  std::map<std::string, SNIApp>* sniApps;
};

class StatusNotifierManager {
  AppContext *ctx;
  std::string SNWXML;
  std::thread ssnThread;

public:
  std::vector<RemoveCallback> removeCallbacks;
  std::map<std::string, StatusApp> registeredItems;
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
  
  // Functions to perform the action
  void executeMenuAction(const std::string &itemService, const std::string &menuPath, u_int32_t timestamp, int actionIndex);

  // DBus Message Handler for Status Notifier Watcher Interface
  void handleRegisterStatusNotifierHost(DBusMessage *msg);
  void handleRegisterStatusNotifierItem(DBusMessage *msg);
};
