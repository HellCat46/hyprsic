#pragma once

#include "../../../app/context.hpp"
#include "glib.h"
#include "gtk/gtk.h"
#include "manager.hpp"
#include <sys/types.h>

struct ChgWSArgs {
  HyprWSManager *wsInstance;
  unsigned int wsId;
  std::string name;
};

struct UpdateWSData {
  HyprWSManager *wsInstance;
  GtkWidget *wsWid;
  GtkWidget *spWSWid;
  unsigned char monitorId;
};

class HyprWSModule {
  HyprWSManager *hyprInstance;
  GtkWidget *wsWid;
  GtkWidget *spWSWid;
  LoggingManager *logger;
  
  unsigned char monitorId;

public:
  HyprWSModule(AppContext *ctx, HyprWSManager *hyprInstance);

  GtkWidget* setup(unsigned char monitorId);
  static void updateWorkspaces(HyprWSManager *hyprInstance, GtkWidget *wsBox,
                               GtkWidget *spWSBox, unsigned char monitorId);
  static void chgWS(GtkWidget *widget, GdkEvent *e, gpointer user_data);
  static void chgSPWS(GtkWidget *widget, GdkEvent *e, gpointer user_data);
  static void handleWSScroll(GtkWidget* widget, GdkEventScroll* e, gpointer user_data);
  
  
  // Update Workspace UI Function
  static gboolean updateWorkspaceUI(gpointer data);
};
