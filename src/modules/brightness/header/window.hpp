#pragma once

#include "gtkmm/adjustment.h"
#include "gtkmm/box.h"
#include "gtkmm/label.h"
#include "gtkmm/scale.h"
#include "manager.hpp"
#include "services/header/comm_bus.hpp"
#include "services/header/context.hpp"

class BrightnessWindow {
  AppContext *ctx;
  BrightnessManager *manager;
  CommunicationBus *commBus;

  Gtk::Box winBox;
  Glib::RefPtr<Gtk::Adjustment> adjWid;
  Gtk::Scale scale;
  Gtk::Label lbl;

  void handleScaleChange(double value);

public:
  BrightnessWindow(AppContext *ctx, CommunicationBus *commBus,
                   BrightnessManager *manager);
  void init();
  void update();

 };
