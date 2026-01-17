#pragma once
#include "../../app/context.hpp"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "gtk/gtk.h"
#include <functional>
#include <string>
#include <thread>
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
  std::thread notifThread;

  void captureNotification(
      std::function<void(NotifFuncArgs *)> showNotification);

  // Notification Daemon Responses to Messages
  Notification handleNotifyCall(DBusMessage *msg);
  void handleGetCapabilitiesCall(DBusMessage *msg);
  void handleGetServerInformationCall(DBusMessage *msg);
  void handleCloseNotificationCall(DBusMessage *msg);

  GdkPixbuf *parseImageData(DBusMessageIter *hintsIter);

public:
  NotificationManager(AppContext *ctx);
  void RunService(std::function<void(NotifFuncArgs *)> showNotification);
};
