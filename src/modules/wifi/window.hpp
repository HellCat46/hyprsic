#pragma once

#include "glib-object.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "manager.hpp"
#include "services/header/context.hpp"
#include <string>

struct ActionArgs {
  WifiManager *manager;
  std::string devPath;
};

class WifiWindow {
  AppContext *ctx;
  WifiManager *manager;
  GtkWidget *mainBox;

  GtkWidget *scanBtn;
  GtkWidget *powerBtn;

  GtkWidget *connDevBox;
  GtkWidget *connDevIBox;
  GtkWidget *connDeviceName;
  GtkWidget *frgtBtn;
  glong connDevFrgtId;

  GtkWidget *devBox;
  GtkWidget *devListBox;
  
  GtkWidget *passEntBox;
  GtkWidget *passEntry;

  static void handleScan(GtkWidget *widget, gpointer user_data);
  static void handleConnect(GtkWidget *widget, gpointer user_data);
  static void handleDisconnect(GtkWidget *widget, gpointer user_data);
  static void handleForget(GtkWidget *widget, gpointer user_data);
  static void handlePassSubmit(GtkWidget *widget, gpointer user_data);
  
  static void FreeActionArgs(gpointer data, GClosure *closure);

  void updateConnDev();
  GtkWidget* addDevList(const std::string &devPath, const WifiStation &station);
  void addTooltip(GtkWidget *widget, const WifiStation &station);

public:
  WifiWindow(AppContext *context, WifiManager *manager);
  void init();
  void update();
};
