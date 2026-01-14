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
  DBManager* dbManager;

  GtkWidget *menuWin;
  GtkWidget *scrollWinBox;
public:
  NotificationModule(AppContext *ctx);

  // Notification List Functions
  void setup(GtkWidget *box);
  void update();

  static void chgVisibiltyWin(GtkWidget *widget, gpointer user_data);
  static void deleteNotificationCb(GtkWidget *widget, gpointer user_data);
  
  // Notification PopUp Functions
  static void showNotification(NotifFuncArgs *args);
  static gboolean autoCloseNotificationCb(gpointer user_data);
  static void closeNotificationCb(GtkWidget *widget, gpointer user_data);
};
