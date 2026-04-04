#pragma once

#include "glib.h"
#include "gtk/gtk.h"
#include "manager.hpp"
#include "window.hpp"

class NotificationModule {
  NotificationManager *manager;
  NotificationWindow *window;
  AppContext *ctx;
  CommunicationBus *commBus;

public:
  NotificationModule(AppContext *ctx, CommunicationBus *commBus,
                     NotificationManager *notifInstance,
                     NotificationWindow *window);

  // Notification List Functions
  GtkWidget *setup();

  static void chgVisibiltyWin(GtkWidget *widget, GdkEvent *e,
                              gpointer user_data);
};
