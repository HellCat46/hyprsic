#pragma once

#include "../../app/context.hpp"
#include "manager.hpp"

class NotificationWindow {
  NotificationManager *manager;
  AppContext *ctx;
  DBManager *dbManager;

  GtkWidget *menuBox;
  GtkWidget *scrollWinBox;

  std::unordered_map<std::string, NotifListItem> notifLookup;

public:
  NotificationWindow(AppContext *ctx, NotificationManager *manager);
  void init();
  void update(bool force = false);
  
  
  static void deleteNotificationCb(GtkWidget *widget, gpointer user_data);
  static void handleDndToggle(GtkSwitch *widget, gboolean state,
                              gpointer user_data);
  static void handleClearAll(GtkWidget *widget, gpointer user_data);

  // Notification PopUp Functions
  static void showNotification(NotifFuncArgs *args);
  static void autoCloseNotificationCb(gpointer user_data);
  static void closeNotificationCb(GtkWidget *widget, gpointer user_data);
};
