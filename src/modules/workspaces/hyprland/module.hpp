#pragma once

#include "../../../app/context.hpp"
#include "glib.h"
#include "gtk/gtk.h"
#include "manager.hpp"

struct ChgWSArgs {
  HyprWSManager *wsInstance;
  unsigned int wsId;
  std::string name;
};

struct UpdateWSData {
  HyprWSManager *wsInstance;
  GtkWidget *wsWid;
  GtkWidget *spWSWid;
};

class HyprWSModule {
  HyprWSManager *hyprInstance;
  GtkWidget *wsWid;
  GtkWidget *spWSWid;
  LoggingManager *logger;

public:
  HyprWSModule(AppContext *ctx, HyprWSManager *hyprInstance);

  void setup(GtkWidget *main_box);
  static void updateWorkspaces(HyprWSManager *hyprInstance, GtkWidget *wsBox,
                               GtkWidget *spWSBox);
  static void chgWS(GtkWidget *widget, GdkEvent *e, gpointer user_data);
  static void chgSPWS(GtkWidget *widget, GdkEvent *e, gpointer user_data);
  
  
  // Update Workspace UI Function
  static gboolean updateWorkspaceUI(gpointer data);
};
