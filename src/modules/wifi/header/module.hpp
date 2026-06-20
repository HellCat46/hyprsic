#pragma once

#include "gtkmm/label.h"
#include "services/header/context.hpp"
#include "manager.hpp"

class WifiModule {
  AppContext *ctx;
  WifiManager *manager;

  Gtk::Label mainLbl;

public:
  WifiModule(AppContext *context, WifiManager *manager);
  Gtk::Label& setup();
  void update();

};
