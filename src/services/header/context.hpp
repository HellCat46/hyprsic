#pragma once
#include "database.hpp"
#include "dbus/dbus.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "gdk/gdk.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "resources/store.hpp"
#include "services/header/logging.hpp"

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
  BATTERY,
  SYSINFO
};

struct Notification {
  std::string id;
  std::string app_name;
  uint32_t replaces_id;
  std::string app_icon;
  std::string summary;
  std::string body;
  GdkPixbuf *icon_pixbuf;
  int32_t expire_timeout;
};

struct NotifListItem {
  std::list<NotificationRecord>::iterator it;
  GtkWidget *widget;
};

struct NotifFuncArgs {
  char *notifId;
  Notification *notif;
  std::unordered_map<std::string, GtkWidget *> *notifications;
  std::unordered_map<std::string, NotifListItem> *notifLookup;
  LoggingManager *logger;
  DBManager *dbManager;
  bool dnd;
};

class AppContext {
  GtkWidget *updateWin;
  GtkWidget *ctrlWin;
  GtkWidget *notifWin;

  // Notification Window Manager
  GtkWidget *notifEvtBox;
  GtkWidget *notifLogo;
  GtkWidget *notifTitle;
  GtkWidget *notifBody;
  gulong closeNotifId;

  GtkGrid *updateWinGrid;
  GtkWidget *updateIcon;
  GtkWidget *updateMsg;
  guint updateTimeoutId;

  static void hideUpdateWindow(gpointer user_data);
  static gboolean handleKeyPress(GtkWidget *wid, guint keyval, guint keycode,
                                 GdkModifierType state, gpointer data);

  // Notification Window Functions
  static void showNotification(NotifFuncArgs *args);
  static void autoCloseNotificationCb(gpointer user_data);
  static void closeNotificationCb(GtkWidget *widget, GdkEvent *e, gpointer user_data);

  // Setup Windows
  void setupUpdateWindow();
  void setupNotifWindow();
  void setupCtrlWindow();

public:
  DbusSystem dbus;
  DBManager dbManager;
  LoggingManager logger;
  ResourceStore resStore;
  GtkWidget *moduleStk;

  AppContext();
  void initWindows();
  bool showUpdateWindow(UpdateModule module, std::string type, std::string msg);
  void showCtrlWindow(const std::string &moduleName, gint width = -1,
                      gint height = -1);
  void showNotifWindow(Notification *notif, bool dnd);
  void addModule(GtkWidget *moduleBox, const std::string &moduleName);
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

struct NotifWindowData {
  NotificationRecord notif;
  AppContext *ctx;
  bool dnd;
};