#pragma once

#include "glib.h"
#include "gtk/gtk.h"
#include "manager.hpp"

class NotificationModule {
  NotificationManager* notifInstance;
  AppContext *ctx;
  DBManager* dbManager;

  GtkWidget *menuWin;
  GtkWidget *scrollWinBox;
public:
  NotificationModule(AppContext *ctx, NotificationManager *notifInstance);

  // Notification List Functions
  void setup(GtkWidget *box);
  void update();

  static void chgVisibiltyWin(GtkWidget *widget, GdkEvent *e, gpointer user_data);
  static void deleteNotificationCb(GtkWidget *widget, gpointer user_data);
  
  static void handleDndToggle(GtkSwitch *widget, gboolean state, gpointer user_data);
  
  // Notification PopUp Functions
  static void showNotification(NotifFuncArgs *args);
  static void autoCloseNotificationCb(gpointer user_data);
  static void closeNotificationCb(GtkWidget *widget, gpointer user_data);
};
