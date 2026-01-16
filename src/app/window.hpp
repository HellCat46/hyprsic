#pragma once

#include "glib.h"
#include <gtk-layer-shell.h>
#include <gtk/gtk.h>
#include <vector>

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

struct Window {
    GtkWidget *window = nullptr;
  
    GtkWidget *netWid;
    GtkWidget *diskWid;
    GtkWidget *loadWid;
    GtkWidget *memWid;
    GtkWidget *batteryWid;
    GtkWidget *timeWid;
    
    MprisModule* mprisModule;
    HyprWSModule* hyprModule;
};

class MainWindow {
  GtkApplication *app = nullptr;
  std::vector<Window> mainWindows;
  AppContext ctx;


  Stats stat;
  Memory mem;
  SysLoad load;
  BatteryInfo battery;
  // PlayingNow playing;

  BluetoothModule btModule;
  NotificationModule notifModule;
  MprisManager mprisManager;
  ScreenSaverModule screenSaverModule;

public:
  MainWindow();
  void RunApp();
  static gboolean UpdateData(gpointer data);
  static void activate(GtkApplication *app, gpointer user_data);
};
