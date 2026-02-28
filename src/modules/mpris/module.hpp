#pragma once

#include "gdk/gdk.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "manager.hpp"
#include "window.hpp"

class MprisModule {
  MprisManager* manager;
  MprisWindow* window;
  AppContext *ctx;

  GtkWidget *mainLbl;


public:
  MprisModule(AppContext *ctx, MprisManager* mprisMgr, MprisWindow* mprisWindow);

  GtkWidget* setup();
  void update();

  static void chgVisibilityMenu(GtkWidget *widget, GdkEvent *e,
                              gpointer user_data);
};
