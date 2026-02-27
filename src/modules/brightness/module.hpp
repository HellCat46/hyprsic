#pragma once

#include "glib.h"
#include "gtk/gtk.h"
#include "manager.hpp"

class BrightnessModule {
  BrightnessManager *manager;
  AppContext *ctx;
  GtkWidget *mainWid;
  GtkWidget *mainWin;
  GtkAdjustment *adjWid;

public:
  BrightnessModule(BrightnessManager *manager, AppContext *ctx);
  GtkWidget *setup();
  void update();

  static void handleWinOpen(GtkWidget *wid, GdkEventButton* evt, gpointer data);
  static void handleScaleChange(GtkRange *range, GtkScrollType *scroll,
                                          gdouble value, gpointer data);
};
