#pragma once
#include "manager.hpp"
#include "window.hpp"



class BluetoothModule {
  AppContext *ctx;
  BluetoothManager *manager;
  BluetoothWindow *window;

  static void switchVisibilityBTMenu(GtkWidget *widget, GdkEvent *e,
                                     gpointer user_data);
public:
  BluetoothModule(AppContext *ctx, BluetoothManager *manager,
                  BluetoothWindow *window);
  // UI Prep Functions
  GtkWidget *setup();
};
