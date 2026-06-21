#pragma once

#include "gtkmm/label.h"
#include "services/header/context.hpp"
#include "manager.hpp"

class WifiModule {
  AppContext *ctx;
  WifiManager *manager;

  Gtk::Box mainBox;
  Gtk::Image icon;
  Gtk::Label lbl;

public:
  WifiModule(AppContext *context, WifiManager *manager);
  Gtk::Box& setup();
  void update();

};
