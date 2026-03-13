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
  Notification handleNotifyCall(DBusMessage *msg);
  void handleGetCapabilitiesCall(DBusMessage *msg);
  void handleGetServerInformationCall(DBusMessage *msg);
  void handleCloseNotificationCall(DBusMessage *msg);

  // Helper Functions
  GdkPixbuf *parseImageData(DBusMessageIter *hintsIter);

public:
  bool dnd;
  NotificationManager(AppContext *ctx);

  void setupDBus();
  void handleDbusMessage(DBusMessage *msg);
};
