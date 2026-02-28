#pragma once

#include "../../app/context.hpp"
#include "manager.hpp"

class BrightnessWindow {
  AppContext *ctx;
  BrightnessManager *manager;

  GtkWidget *winBox;
  GtkAdjustment *adjWid;

public:
  BrightnessWindow(AppContext *ctx, BrightnessManager *manager);
  void init();
  void update();

  static void handleScaleChange(GtkRange *range, GtkScrollType *scroll,
                                gdouble value, gpointer data);
};
