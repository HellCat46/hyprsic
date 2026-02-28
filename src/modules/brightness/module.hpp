#pragma once

#include "glib.h"
#include "gtk/gtk.h"
#include "manager.hpp"
#include "window.hpp"

class BrightnessModule {
  BrightnessManager *manager;
  BrightnessWindow *window;
  AppContext *ctx;
  
  GtkWidget *mainWid;

public:
  BrightnessModule(AppContext *ctx, BrightnessManager *manager, BrightnessWindow *window);
  GtkWidget *setup();
  void update();

  static void handleWinOpen(GtkWidget *wid, GdkEventButton* evt, gpointer data);

};
