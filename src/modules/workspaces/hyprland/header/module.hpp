#pragma once

#include "glib.h"
#include "gtk/gtk.h"
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
  GtkWidget *wsWid;
  GtkWidget *spWSWid;
  unsigned char monitorId;
};

class HyprWSModule {
  HyprWSManager *hyprInstance;
  LoggingManager *logger;
  CommunicationBus *commBus;

  GtkWidget *wsWid;
  GtkWidget *spWSWid;

  unsigned char monitorId;


public:
  HyprWSModule(AppContext *ctx, CommunicationBus *commBus,
               HyprWSManager *hyprInstance);

  GtkWidget *setup(unsigned char monitorId);
  static void updateWorkspaces(CommunicationBus *commBus,
                               HyprWSManager *hyprInstance, GtkWidget *wsBox,
                               GtkWidget *spWSBox, unsigned char monitorId);

  static void chgWS(GtkWidget *widget, GdkEvent *e, gpointer user_data);
  static void chgSPWS(GtkWidget *widget, GdkEvent *e, gpointer user_data);
  static void handleWSScroll(GtkWidget *widget, GdkEventScroll *e,
                             gpointer user_data);

  // Update Workspace UI Function
  static gboolean updateWorkspaceUI(gpointer data);
};
