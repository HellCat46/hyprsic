#pragma once

#include "gtkmm/box.h"
#include "gtkmm/image.h"
#include "gtkmm/label.h"
#include "manager.hpp"
#include "window.hpp"

class BrightnessModule {
  AppContext *ctx;
  BrightnessManager *manager;
  BrightnessWindow *window;

  Gtk::Box mainBox;
  Gtk::Image mainImg;
  Gtk::Label mainLbl;

public:
  BrightnessModule(AppContext *ctx, BrightnessManager *manager);
  Gtk::Box& setup();
  void update();
};
