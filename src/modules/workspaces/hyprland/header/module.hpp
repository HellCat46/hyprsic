#pragma once

#include "gtkmm/box.h"
#include "manager.hpp"
#include "services/header/comm_bus.hpp"
#include "services/header/context.hpp"
#include <sys/types.h>

struct ChgWSArgs {
  CommunicationBus *commBus;
  unsigned int wsId;
  std::string name;
};

struct UpdateWSData {
  CommunicationBus *commBus;
  HyprWSManager *wsInstance;
  Gtk::Box &wsBox;
  Gtk::Box &spWSBox;
  unsigned char monitorId;
};

class HyprWSModule {
  HyprWSManager *hyprInstance;
  LoggingManager *logger;
  CommunicationBus *commBus;
  
  Gtk::Box mainBox;
  Gtk::Box wsBox;
  Gtk::Box spWSBox;

  unsigned char monitorId;

public:
  HyprWSModule(AppContext *ctx, CommunicationBus *commBus,
               HyprWSManager *hyprInstance);

  Gtk::Box &setup(unsigned char monitorId);
  void updateWorkspaces();

  void chgWS(unsigned int wsId);
  void chgSPWS(unsigned int id, std::string name);
  void handleWSScroll(double dy);

  // Update Workspace UI Function
  void updateWorkspaceUI();
};
