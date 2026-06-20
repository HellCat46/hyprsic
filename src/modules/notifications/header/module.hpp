#pragma once

#include "gtkmm/label.h"
#include "manager.hpp"
#include "window.hpp"

class NotificationModule {
  NotificationManager *manager;
  NotificationWindow *window;
  AppContext *ctx;
  CommunicationBus *commBus;

  Gtk::Label mainLbl;

public:
  NotificationModule(AppContext *ctx, CommunicationBus *commBus,
                     NotificationManager *notifInstance);

  // Notification List Functions
  Gtk::Label& setup();
};
