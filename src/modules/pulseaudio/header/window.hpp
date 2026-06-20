#pragma once

#include "gtkmm/button.h"
#include "gtkmm/dropdown.h"
#include "gtkmm/scale.h"
#include "manager.hpp"
#include "services/header/comm_bus.hpp"
#include "services/header/context.hpp"
#include <cstdint>
#include <vector>

class PulseAudioWindow {
  AppContext *ctx;
  PulseAudioManager *manager;
  CommunicationBus *commBus;

  Gtk::Box mainBox;

  Gtk::Button outMuteBtn;
  Gtk::Scale outScale;
  Gtk::DropDown outDropdown;

  Gtk::Button inMuteBtn;
  Gtk::Scale inScale;
  Gtk::DropDown inDropdown;

  void updateControls(bool mute, bool isOutput, const std::vector<uint32_t> &volumes, Gtk::Button &btn, Gtk::Scale &scale);

  void chgDevice(bool isOutput);
  void handleChgVolume(double value, bool isOutput);

public:
  PulseAudioWindow(AppContext *ctx, CommunicationBus *commBus,
                   PulseAudioManager *manager);
  void init();
  void update();

  // Also Used by CLI controller
  void toggleMute(bool isOutput);
};
