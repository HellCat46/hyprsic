#pragma once

#include "gtkmm/image.h"
#include "gtkmm/label.h"
#include "manager.hpp"
#include "services/header/comm_bus.hpp"

class NotificationModule {
  NotificationManager *manager;
  AppContext *ctx;
  CommunicationBus *commBus;

  Gtk::Box mainBox;
  Gtk::Image mainIcon;
  Gtk::Label mainLbl;

public:
  NotificationModule(AppContext *ctx, CommunicationBus *commBus,
                     NotificationManager *notifInstance);

  // Notification List Functions
  Gtk::Box& setup();
  void update();
};
