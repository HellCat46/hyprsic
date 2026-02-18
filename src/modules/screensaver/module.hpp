#pragma once

#include "gtk/gtk.h"
#include "manager.hpp"

class ScreenSaverModule {
  ScreenSaverManager *screenSaverMgr;
  LoggingManager *logger;

  GtkWidget *btnWid;

public:
  ScreenSaverModule(AppContext *ctx, ScreenSaverManager *scrnsavrInstance);
  GtkWidget* setup();

  static void switchScreenSaverCb(GtkWidget *widget, GdkEvent *e, gpointer user_data);
};
