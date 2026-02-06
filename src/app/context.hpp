#pragma once
#include "../database/db_manager.hpp"
#include "../logging/manager.hpp"
#include "dbus/dbus.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "glib.h"
#include "gtk/gtk.h"

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
  WIFI
};

class AppContext {
  GtkWidget *updateWindow;
  GtkGrid *updateWinGrid;
  GtkWidget *updateIcon;
  GtkWidget *updateMsg;
  guint updateTimeoutId;
  
  static void hideUpdateWindow(gpointer user_data);
public:
  DbusSystem dbus;
  DBManager dbManager;
  LoggingManager logger;
  AppContext();

  void initUpdateWindow();
  bool showUpdateWindow(UpdateModule module, std::string type, std::string msg);
};

struct UpdateWindowData {
    UpdateModule module;
    std::string type, msg;
    AppContext* ctx;
    GdkPixbuf* pixBuf;
};
