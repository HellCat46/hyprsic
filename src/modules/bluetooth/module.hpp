#pragma once
#include "glib.h"
#include "gtk/gtk.h"
#include "manager.hpp"

struct FuncArgs {
  char *devIfacePath;
  bool state;
  BluetoothManager *btManager;
  AppContext *ctx;
};

class BluetoothModule {
  BluetoothManager *btManager;
  AppContext *ctx;
  GtkWidget *powerBtn;
  GtkWidget *scanBtn;
  GtkWidget *menuWin;
  GtkWidget *devBox;

  GtkWidget *availDevTitle;
  GtkWidget *availDevList;
  GtkWidget *pairedDevTitle;
  GtkWidget *pairedDevList;

  void addDeviceEntry(const Device &dev, GtkWidget *parentBox, bool isPaired);
  static void FreeArgs(gpointer data);

public:
  BluetoothModule(AppContext *ctx, BluetoothManager *manager);
  // UI Prep Functions
  void setupBT(GtkWidget *box);
  void updateBTList(bool force = false);

  static void switchVisibilityBTMenu(GtkWidget *widget, GdkEvent *e,
                                     gpointer user_data);
  static void handleDiscovery(GtkWidget *widget, gpointer user_data);
  static void handlePower(GtkSwitch *widget, gboolean state,
                          gpointer user_data);
  static void handleDeviceConnect(GtkWidget *widget, gpointer user_data);
  static void handleDeviceTrust(GtkWidget *widget, gpointer user_data);
  static void handleDeviceRemove(GtkWidget *widget, gpointer user_data);
};
