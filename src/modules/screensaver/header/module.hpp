#pragma once

#include "gtk/gtk.h"
#include "manager.hpp"
#include "services/header/comm_bus.hpp"

class ScreenSaverModule {
  ScreenSaverManager *screenSaverMgr;
  CommunicationBus *commBus;
  LoggingManager *logger;

  GtkWidget *btnWid;

public:
  ScreenSaverModule(AppContext *ctx, CommunicationBus *commBus,
                    ScreenSaverManager *scrnsavrInstance);
  GtkWidget *setup();

  static void switchScreenSaverCb(GtkWidget *widget, GdkEvent *e,
                                  gpointer user_data);
};
