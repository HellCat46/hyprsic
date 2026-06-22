#pragma once

#include "giomm/liststore.h"
#include "glibmm/object.h"
#include "glibmm/refptr.h"
#include "gtkmm/button.h"
#include "gtkmm/dropdown.h"
#include "gtkmm/scale.h"
#include "gtkmm/signallistitemfactory.h"
#include "manager.hpp"
#include "services/header/comm_bus.hpp"
#include "services/header/context.hpp"
#include <cstdint>
#include <vector>

class PulseAudioDeviceObject : public Glib::Object {
public:
  std::string devName;
  PulseAudioDevice data;

  static Glib::RefPtr<PulseAudioDeviceObject>
  create(const std::string &devName, const PulseAudioDevice &data);

  PulseAudioDeviceObject(const std::string &devName,
                         const PulseAudioDevice &data)
      : devName(devName), data(data) {}
};

class PulseAudioWindow {
  AppContext *ctx;
  PulseAudioManager *manager;
  CommunicationBus *commBus;

  Gtk::Box mainBox;
  Glib::RefPtr<Gtk::SignalListItemFactory> devFactory;

  Gtk::Button outMuteBtn;
  Gtk::Scale outScale;
  Gtk::DropDown outDropdown;
  Glib::RefPtr<Gio::ListStore<PulseAudioDeviceObject>> outStore;

  Gtk::Button inMuteBtn;
  Gtk::Scale inScale;
  Gtk::DropDown inDropdown;
  Glib::RefPtr<Gio::ListStore<PulseAudioDeviceObject>> inStore;

  void updateControls(bool mute, bool isOutput,
                      const std::vector<uint32_t> &volumes, Gtk::Button &btn,
                      Gtk::Scale &scale);

  void chgDevice(bool isOutput);
  void handleChgVolume(double value, bool isOutput);

  // Device Factory Callbacks
  void onFactorySetup(const Glib::RefPtr<Gtk::ListItem>& list_item);
  void onFactoryBind(const Glib::RefPtr<Gtk::ListItem>& list_item);
  

public:
  PulseAudioWindow(AppContext *ctx, CommunicationBus *commBus,
                   PulseAudioManager *manager);
  void init();
  void update();

  // Also Used by CLI controller
  void toggleMute(bool isOutput);
};
