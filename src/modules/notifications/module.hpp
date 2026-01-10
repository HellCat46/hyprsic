#pragma once

#include "glib.h"
#include "gtk/gtk.h"
#include "manager.hpp"
#include <string>
#include <unordered_map>



class NotificationModule {
  std::unordered_map<std::string, GtkWidget *> notifications;
  NotificationManager notifInstance;
  LoggingManager *logger;

public:
  NotificationModule(AppContext *ctx);
  void setup(GtkWidget *box);
  void updateBox();

  static void showNotification(NotifFuncArgs *args);
  static gboolean autoCloseNotificationCb(gpointer user_data);
  static void closeNotificationCb(GtkWidget *widget, gpointer user_data);
};
