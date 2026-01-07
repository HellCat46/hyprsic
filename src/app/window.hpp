#pragma once

#include "glib.h"
#include <gtk-layer-shell.h>
#include <gtk/gtk.h>

#include "../modules/sysinfo/stats.hpp"
#include "../modules/sysinfo/battery.hpp"
#include "../modules/sysinfo/memory.hpp"
#include "../modules/media/playing_now.hpp"
#include "../modules/sysinfo/sys_load.hpp"
#include "../modules/workspaces/hyprland.hpp"
#include "../modules/notifications/notification_manager.hpp"

#include "../modules/bluetooth/bluetooth_manager.hpp"

struct ChgWSArgs {
  HyprWorkspaces* wsInstance;
  unsigned int wsId;
};

class MainWindow {
  GtkApplication *app = nullptr;
  GtkWidget* window = nullptr;
  AppContext ctx;
  
  Stats stat;
  GtkWidget* netWid;
  GtkWidget* diskWid;
  
  SysLoad load;
  GtkWidget* loadWid;
  
  Memory mem;
  GtkWidget* memWid;
  
  BatteryInfo battery;
  GtkWidget* batteryWid;
  
  GtkWidget* timeWid;
    
  PlayingNow playing;
  
  BluetoothManager btManager;
  GtkWidget* btPowerBtn;
  GtkWidget* btScanBtn;
  GtkWidget* btPopOverMenu;
  GtkWidget* btDevList;

  NotificationManager notifManager;
  
  
  
  HyprWorkspaces hyprWS;
  GtkWidget* workspaceSecWid;

public:
  MainWindow();
  void RunApp();
  static gboolean UpdateData(gpointer data);
  static void activate(GtkApplication *app, gpointer user_data);

  // UI Prep Functions
  static void setupBT(GtkWidget* box, MainWindow* self);
  static void showBTMenu(GtkWidget* widget, gpointer user_data);
  static void hideBTMenu(GtkWidget* widget, gpointer user_data);
  static void updateBTList(MainWindow* self);
  static void handleDiscovery(GtkWidget* widget, gpointer user_data);
  static void handlePower(GtkWidget* widget, gpointer user_data);
  
  // UI Hyprland Workspace Functions
  static void setupWorkspaces(HyprWorkspaces* wsInstance, GtkWidget* workspaceBox);
  static void chgWorkspace(GtkWidget* widget, GdkEvent* e, gpointer user_data);
};
