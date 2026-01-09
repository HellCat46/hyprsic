#pragma once

#include "../../../app/context.hpp"
#include "gtk/gtk.h"
#include "manager.hpp"

struct ChgWSArgs {
  HyprWorkspaces *wsInstance;
  unsigned int wsId;
};

class HyprWSModule {
  HyprWorkspaces hyprInstance;
  GtkWidget *SectionWid;
  LoggingManager *logger;

public:
  HyprWSModule(AppContext *ctx);

  void setupWorkspaces(GtkWidget *main_box);
  static void updateWorkspaces(HyprWorkspaces* hyprInstance, GtkWidget* SectionWid);
  static void chgWorkspace(GtkWidget *widget, GdkEvent *e, gpointer user_data);
};
