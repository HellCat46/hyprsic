#pragma once
#include "../database/db_manager.hpp"
#include "../logging/manager.hpp"
#include "dbus/dbus.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "../resources/store.hpp"

class DbusSystem {
public:
  DBusConnection *sysConn;
  DBusError sysErr;

  DBusConnection *ssnConn;
  DBusError ssnErr;

  DbusSystem();
  ~DbusSystem();

  void DictToInt64(DBusMessageIter *iter, uint64_t &outValue);
  void DictToString(DBusMessageIter *iter, std::string &outValue);
};

enum class UpdateModule {
  MPRIS,
  NOTIFICATIONS,
  BLUETOOTH,
  SCREENSAVER,
  PULSEAUDIO,
  WIFI,
  BATTERY
};

class AppContext {
  GtkWidget *updateWindow;
  GtkGrid *updateWinGrid;
  GtkWidget *updateIcon;
  GtkWidget *updateMsg;
  guint updateTimeoutId;
  
  GtkWidget* rbWindow; // Right bottom window, but right now more like ragebait window
  
  static void hideUpdateWindow(gpointer user_data);
public:
  DbusSystem dbus;
  DBManager dbManager;
  LoggingManager logger;
  ResourceStore resStore;
  AppContext();

  void initUpdateWindow();
  bool showUpdateWindow(UpdateModule module, std::string type, std::string msg);
  void switchRBWindow(GtkWidget* win);
};

struct UpdateWindowData {
    UpdateModule module;
    std::string type, msg;
    AppContext* ctx;
    GdkPixbuf* pixBuf;
};
