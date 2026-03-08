#pragma once

#include "gtk/gtk.h"
#include "manager.hpp"
#include "services/header/context.hpp"


class NotificationWindow {
  AppContext *ctx;
  NotificationManager *manager;

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

};
