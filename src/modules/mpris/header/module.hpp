#pragma once

#include "glibmm/refptr.h"
#include "gtkmm/gestureclick.h"
#include "gtkmm/label.h"
#include "manager.hpp"
#include "services/header/comm_bus.hpp"

class MprisModule {
  AppContext *ctx;
  MprisManager *manager;
  CommunicationBus *commBus;

  Gtk::Label mainLbl;
  Glib::RefPtr<Gtk::GestureClick> lblAction;

public:
  MprisModule(AppContext *ctx, MprisManager *mprisMgr,
              CommunicationBus *commBus);

  Gtk::Label &setup();
  void update();

  void chgVisibilityMenu(int, int, double);
};
