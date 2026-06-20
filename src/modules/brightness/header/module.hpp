#pragma once

#include "gtkmm/box.h"
#include "gtkmm/label.h"
#include "manager.hpp"
#include "window.hpp"

class BrightnessModule {
  AppContext *ctx;
  BrightnessManager *manager;
  BrightnessWindow *window;

  Gtk::Label mainLbl;
  Gtk::Box mainBox;

public:
  BrightnessModule(AppContext *ctx, BrightnessManager *manager);
  Gtk::Box& setup();
  void update();
};
