#pragma once

#include "gtkmm/box.h"
#include "gtkmm/button.h"
#include "gtkmm/label.h"
#include "gtkmm/listbox.h"
#include "gtkmm/switch.h"
#include "manager.hpp"
#include "services/header/comm_bus.hpp"
#include "services/header/comm_types.hpp"
#include "services/header/context.hpp"

struct FuncArgs {
  std::string devIfacePath;
  bool state;
  CommunicationBus *commBus;
  AppContext *ctx;
};

class BluetoothWindow {
  AppContext *ctx;
  BluetoothManager *manager;
  CommunicationBus *commBus;

  Gtk::Switch powerBtn;
  Gtk::Button scanBtn;
  Gtk::Box menuBox;
  Gtk::Box devBox;

  Gtk::Label availDevTitle;
  Gtk::ListBox availDevList;
  Gtk::Label pairedDevTitle;
  Gtk::ListBox pairedDevList;

  void addDeviceEntry(const Device &dev, Gtk::ListBox& listBox);

  void handleDiscovery();
  void handlePower(bool state);
  void handleDeviceConnect(bool state, std::string devIfacePath);
  void handleDeviceTrust(bool state, std::string devIfacePath);
  void handleDeviceRemove(std::string devIfacePath);
  
  // Communication Bus Response
  void handleResponse(ResponseMessage);

public:
  BluetoothWindow(AppContext *ctx, CommunicationBus *commBus,
                  BluetoothManager *manager);

  void init();
  void update(bool force = false);
};
