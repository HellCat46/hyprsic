#pragma once

#include "gtkmm/box.h"
#include "manager.hpp"
#include "services/header/comm_bus.hpp"
#include "services/header/context.hpp"

class NotificationWindow : public sigc::trackable {
  AppContext *ctx;
  NotificationManager *manager;
  CommunicationBus *commBus;

  Gtk::Box menuBox;
  Gtk::Box scrollWinBox;

  std::unordered_map<std::string, NotifListItem> notifLookup;

public:
  NotificationWindow(AppContext *ctx, CommunicationBus *commBus,
                     NotificationManager *manager);
  void init();
  void update(bool force = false);

  void deleteNotificationCb(std::string notifId);
  void handleDndToggle(bool state);
  void handleClearAll();
};
