#pragma once
#include "../../app/context.hpp"
#include "dbus/dbus.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "gtk/gtk.h"
#include <functional>
#include <string>
#include <unordered_map>

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

struct NotifFuncArgs {
  char *notifId;
  Notification *notif;
  std::unordered_map<std::string, GtkWidget *> *notifications;
  LoggingManager *logger;
  DBManager *dbManager;
};

class NotificationManager {
  AppContext *ctx;
  std::unordered_map<std::string, GtkWidget *> notifications;

  // Notification Daemon Responses to Messages
  Notification handleNotifyCall(DBusMessage *msg);
  void handleGetCapabilitiesCall(DBusMessage *msg);
  void handleGetServerInformationCall(DBusMessage *msg);
  void handleCloseNotificationCall(DBusMessage *msg);

  // Helper Functions
  GdkPixbuf *parseImageData(DBusMessageIter *hintsIter);

public:
  NotificationManager(AppContext *ctx);

  void setupDBus();
  void handleDbusMessage(DBusMessage* msg, std::function<void(NotifFuncArgs *)> showNotification);
};
