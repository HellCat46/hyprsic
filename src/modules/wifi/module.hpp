#pragma once

#include "../../services/header/context.hpp"
#include "gdk/gdk.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "modules/wifi/manager.hpp"
#include "modules/wifi/window.hpp"
class WifiModule {
  AppContext *ctx;
  WifiManager *manager;
  WifiWindow *window;
  
  GtkWidget* mainLbl;
  
  static void openWindow(GtkWidget* widget, GdkEvent *e, gpointer user_data);

public:
  WifiModule(AppContext *context, WifiManager *manager, WifiWindow *window);
  GtkWidget* setup();
  void update();
  
};