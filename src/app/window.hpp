#pragma once

#include "glib.h"
#include <gtk-layer-shell.h>
#include <gtk/gtk.h>

// #include "../modules/pulseaudio/manager.hpp"
#include "../modules/bluetooth/module.hpp"
#include "../modules/mpris/module.hpp"
#include "../modules/notifications/module.hpp"
#include "../modules/sysinfo/battery.hpp"
#include "../modules/sysinfo/memory.hpp"
#include "../modules/sysinfo/stats.hpp"
#include "../modules/sysinfo/sys_load.hpp"
#include "../modules/workspaces/hyprland/module.hpp"
#include "../modules/screensaver/module.hpp"

class MainWindow {
  GtkApplication *app = nullptr;
  GtkWidget *window = nullptr;
  AppContext ctx;

  Stats stat;
  GtkWidget *netWid;
  GtkWidget *diskWid;

  SysLoad load;
  GtkWidget *loadWid;

  Memory mem;
  GtkWidget *memWid;

  BatteryInfo battery;
  GtkWidget *batteryWid;

  GtkWidget *timeWid;

  // PlayingNow playing;

  BluetoothModule btModule;
  HyprWSModule hyprModule;
  NotificationModule notifModule;
  MprisModule mprisModule;
  ScreenSaverModule screenSaverModule;

public:
  MainWindow();
  void RunApp();
  static gboolean UpdateData(gpointer data);
  static void activate(GtkApplication *app, gpointer user_data);
};
