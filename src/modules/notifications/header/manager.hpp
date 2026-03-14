#pragma once
#include "services/header/context.hpp"
#include "dbus/dbus.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "gtk/gtk.h"
#include <string>
#include <unordered_map>



class NotificationManager {
  AppContext *ctx;
  std::unordered_map<std::string, GtkWidget *> notifications;

  // Notification Daemon Responses to Messages
  Notification handleNotifyCallDbus(DBusMessage *msg);
  void handleGetCapabilitiesCallDbus(DBusMessage *msg);
  void handleGetServerInformationCallDbus(DBusMessage *msg);
  void handleCloseNotificationCallDbus(DBusMessage *msg);

  // Helper Functions
  GdkPixbuf *parseImageData(DBusMessageIter *hintsIter);

public:
  bool dnd;
  NotificationManager(AppContext *ctx);

  void setupDBus();
  void handleDbusMessageDbus(DBusMessage *msg);
};
