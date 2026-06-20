#pragma once

#include "glibmm/refptr.h"
#include "gtkmm/box.h"
#include "gtkmm/label.h"
#include "gtkmm/scale.h"
#include "manager.hpp"
#include "services/header/comm_bus.hpp"
#include "services/header/context.hpp"

class MprisWindow {
  AppContext *ctx;
  MprisManager *manager;
  CommunicationBus *commBus;

  Gtk::Box progBox;
  Gtk::Label progTtl;

  Gtk::Box progBarBox;
  Gtk::Label scaleMin;
  Gtk::Scale scale;
  Gtk::Label scaleMax;

  Glib::RefPtr<Gtk::Adjustment> scaleAdj;

  // Gtk Scale Signal Callbacks
  void handleScaleChange(double value);

  // Track Control Buttons
  void handlePlayPause(int, int, double);
  void handleNextTrack();
  void handlePrevTrack();

public:
  MprisWindow(AppContext *ctx, CommunicationBus *commBus,
              MprisManager *mprisMgr);
  void init();
  void update();

  static std::string timeToStr(uint64_t totalSeconds);
};
