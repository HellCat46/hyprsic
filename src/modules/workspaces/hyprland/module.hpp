#pragma once

#include "../../../app/context.hpp"
#include "gtk/gtk.h"
#include "manager.hpp"

struct ChgWSArgs {
  HyprWSManager *wsInstance;
  unsigned int wsId;
};

struct UpdateWSData {
  unsigned int wsId;
  HyprWSManager *wsInstance;
  GtkWidget *SectionWid;
};

class HyprWSModule {
  HyprWSManager* hyprInstance;
  GtkWidget *SectionWid;
  LoggingManager *logger;

public:
  HyprWSModule(AppContext *ctx, HyprWSManager* hyprInstance);

  void setup(GtkWidget *main_box);
  static void updateWorkspaces(HyprWSManager* hyprInstance, GtkWidget* SectionWid);
  static void chgWorkspace(GtkWidget *widget, GdkEvent *e, gpointer user_data);
};
