#pragma once

#include "gtkmm/button.h"
#include "gtkmm/entry.h"
#include "gtkmm/label.h"
#include "gtkmm/widget.h"
#include "manager.hpp"
#include "services/header/comm_bus.hpp"
#include "services/header/context.hpp"
#include <string>

struct ActionArgs {
  CommunicationBus *commBus;
  std::string devPath;
};

class WifiWindow {
  AppContext *ctx;
  WifiManager *manager;
  CommunicationBus *commBus;
  Gtk::Box mainBox;

  Gtk::Button scanBtn;
  Gtk::Button powerBtn;

  Gtk::Box connDevBox;
  Gtk::Box connDevIBox;
  Gtk::Label connDeviceName;
  Gtk::Button frgtBtn;
  Gtk::Button disCBtn;
  
  Gtk::Box devBox;
  Gtk::Box devListBox;

  Gtk::Box passEntBox;
  Gtk::Entry passEntry;

  void handleScan();
  void handleConnect(std::string devPath);
  void handleDisconnect();
  void handleForget(std::string devPath);
  void handlePassSubmit();

  void updateConnDev();
  void addDevList(Gtk::Box &devListBox, const std::string &devPath, const WifiStation &station);

public:
  WifiWindow(AppContext *context, CommunicationBus *commBus,
             WifiManager *manager);
  void init();
  void update();

  static void addTooltip(Gtk::Widget &widget, const WifiStation &station);
};
