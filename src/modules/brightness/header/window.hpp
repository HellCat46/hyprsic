#pragma once

#include "manager.hpp"
#include "services/header/comm_bus.hpp"
#include "services/header/context.hpp"

class BrightnessWindow {
  AppContext *ctx;
  BrightnessManager *manager;
  CommunicationBus *commBus;

  GtkWidget *winBox;
  GtkAdjustment *adjWid;

public:
  BrightnessWindow(AppContext *ctx, CommunicationBus *commBus,
                   BrightnessManager *manager);
  void init();
  void update();

  static void handleScaleChange(GtkRange *range, GtkScrollType *scroll,
                                gdouble value, gpointer data);
};
