#pragma once

#include "gtkmm/box.h"
#include "gtkmm/image.h"
#include "manager.hpp"
#include "services/header/context.hpp"

class PulseAudioModule {
  PulseAudioManager *manager;
  AppContext *ctx;
  
  Gtk::Box menuBox;
  Gtk::Image barInIcon;
  Gtk::Image barOutIcon;

  bool setupComp;

public:
  PulseAudioModule(PulseAudioManager *paManager, AppContext *ctx);
  Gtk::Box &setup();

  void update();
};
