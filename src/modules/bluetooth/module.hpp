#pragma once
#include "manager.hpp"
#include "gtk/gtk.h"

struct FuncArgs {
  char *devIfacePath;
  bool state;
  BluetoothManager *btManager;
  AppContext *ctx;
};

class BluetoothModule {
  BluetoothManager* btManager;
  AppContext *ctx;
  GtkWidget *btPowerBtn;
  GtkWidget *btScanBtn;
  GtkWidget *btMenuWin;
  GtkWidget *btDevList;

public:
  BluetoothModule(AppContext *ctx, BluetoothManager* manager);
  // UI Prep Functions
  void setupBT(GtkWidget *box);
  void updateBTList();
  
  static void switchVisibilityBTMenu(GtkWidget *widget, GdkEvent *e, gpointer user_data);
  static void handleDiscovery(GtkWidget *widget, gpointer user_data);
  static void handlePower(GtkWidget *widget, gpointer user_data);
  static void handleDeviceConnect(GtkWidget *widget, gpointer user_data);
};
