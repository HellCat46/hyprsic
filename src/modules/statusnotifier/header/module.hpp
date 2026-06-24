#pragma once

#include "gtkmm/box.h"
#include "manager.hpp"
#include "services/header/comm_bus.hpp"
#include <cstdint>
#include <string>

struct MenuActionArgs {
  LoggingManager *logger;
  StatusNotifierManager *snManager;
  CommunicationBus *commBus;
  std::string itemId;
  SNIApp sniApp;
};

struct EvtBtnPressArgs {
  LoggingManager *logger;
  StatusNotifierManager *snManager;
  CommunicationBus *commBus;
  std::string itemId;
  SNIApp sniApp;
  uint32_t evtIdx;
};

class StatusNotifierModule {
  LoggingManager *logger;
  StatusNotifierManager *snManager;
  CommunicationBus *commBus;

  Gtk::Box sniBox;
  std::map<std::string, SNIApp> sniApps;

  void remove(std::string servicePath);

  void handleContextMenuOpen(std::string servicePath);
  void handleEvtButtonPress(std::string servicePath, uint32_t evtIdx, uint32_t timestamp);

public:
  StatusNotifierModule(AppContext *ctx, CommunicationBus *commBus,
                       StatusNotifierManager *snManager);

  Gtk::Box &setup();
  void update();
};
