#pragma once

#include "gtk/gtk.h"
#include "gtkmm/label.h"
#include "manager.hpp"
#include "services/header/comm_bus.hpp"

class ScreenSaverModule {
  ScreenSaverManager *screenSaverMgr;
  CommunicationBus *commBus;
  LoggingManager *logger;

  Gtk::Label mainLbl;

public:
  ScreenSaverModule(AppContext *ctx, CommunicationBus *commBus,
                    ScreenSaverManager *scrnsavrInstance);
  Gtk::Label& setup();

  static void switchScreenSaverCb(GtkWidget *widget, GdkEvent *e,
                                  gpointer user_data);
};
