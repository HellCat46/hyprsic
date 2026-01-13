#pragma once
#include "manager.hpp"
#include "gtk/gtk.h"

struct FuncArgs {
  char *devIfacePath;
  bool state;
  BluetoothManager *btManager;
};

class BluetoothModule {
  BluetoothManager btManager;
  LoggingManager *logger;
  GtkWidget *btPowerBtn;
  GtkWidget *btScanBtn;
  GtkWidget *btMenuWin;
  GtkWidget *btDevList;

public:
  BluetoothModule(AppContext *ctx);
  // UI Prep Functions
  void setupBT(GtkWidget *box);
  void updateBTList();
  
  static void switchVisibilityBTMenu(GtkWidget *widget, gpointer user_data);
  static void handleDiscovery(GtkWidget *widget, gpointer user_data);
  static void handlePower(GtkWidget *widget, gpointer user_data);
  static void handleDeviceConnect(GtkWidget *widget, gpointer user_data);
};
