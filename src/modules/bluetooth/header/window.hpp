#pragma once

#include "manager.hpp"
#include "services/header/comm_bus.hpp"
#include "services/header/context.hpp"

struct FuncArgs {
  std::string devIfacePath;
  bool state;
  CommunicationBus *commBus;
  AppContext *ctx;
};

class BluetoothWindow {
  AppContext *ctx;
  BluetoothManager *manager;
  CommunicationBus *commBus;

  GtkWidget *powerBtn;
  GtkWidget *scanBtn;
  GtkWidget *menuBox;
  GtkWidget *devBox;

  GtkWidget *availDevTitle;
  GtkWidget *availDevList;
  GtkWidget *pairedDevTitle;
  GtkWidget *pairedDevList;

  void addDeviceEntry(const Device &dev, GtkWidget *parentBox);
  static void FreeArgs(gpointer data, GClosure *closure);

  static void handleDiscovery(GtkWidget *widget, gpointer user_data);
  static void handlePower(GtkSwitch *widget, gboolean state,
                          gpointer user_data);
  static void handleDeviceConnect(GtkWidget *widget, gpointer user_data);
  static void handleDeviceTrust(GtkWidget *widget, gpointer user_data);
  static void handleDeviceRemove(GtkWidget *widget, gpointer user_data);

public:
  BluetoothWindow(AppContext *ctx, CommunicationBus *commBus,
                  BluetoothManager *manager);

  void init();
  void update(bool force = false);
};
