#pragma once
#include "../database/db_manager.hpp"
#include "../logging/manager.hpp"
#include "../resources/store.hpp"
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
  WIFI,
  BATTERY
};

class AppContext {
  GtkWidget *updateWin;
  GtkWidget *ctrlWin;

  GtkGrid *updateWinGrid;
  GtkWidget *updateIcon;
  GtkWidget *updateMsg;
  guint updateTimeoutId;

  static void hideUpdateWindow(gpointer user_data);
  static gboolean handleKeyPress(GtkWidget *wid, guint keyval, guint keycode,
                                 GdkModifierType state, gpointer data);

public:
  DbusSystem dbus;
  DBManager dbManager;
  LoggingManager logger;
  ResourceStore resStore;
  GtkWidget* moduleStk;

  AppContext();
  void initWindows();
  bool showUpdateWindow(UpdateModule module, std::string type, std::string msg);
  void showCtrlWindow(const std::string& moduleName, gint width = -1, gint height = -1);
  void addModule(GtkWidget *moduleBox, const std::string& moduleName);
};

struct CtrlWindowData {
  std::string moduleName;
  gint width, height;
  AppContext *ctx;
};

struct UpdateWindowData {
  UpdateModule module;
  std::string type, msg;
  AppContext *ctx;
  GdkPixbuf *pixBuf;
};
