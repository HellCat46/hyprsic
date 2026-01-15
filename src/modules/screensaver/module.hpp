#pragma once

#include "gtk/gtk.h"
#include "manager.hpp"

class ScreenSaverModule {
  ScreenSaverManager *screenSaverMgr;
  LoggingManager *logger;

  GtkWidget *btnWid;

public:
  ScreenSaverModule(AppContext *ctx);
  void setup(GtkWidget *box);

  static void switchScreenSaverCb(GtkWidget *widget, gpointer user_data);
};
