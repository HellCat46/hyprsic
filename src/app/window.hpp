#pragma once

#include "context.hpp"
#include "glib.h"
#include <gtk-layer-shell.h>
#include <gtk/gtk.h>
#include <memory>
#include <vector>

// #include "../modules/pulseaudio/manager.hpp"
#include "../modules/bluetooth/module.hpp"
#include "../modules/mpris/module.hpp"
#include "../modules/notifications/module.hpp"
#include "../modules/screensaver/module.hpp"
#include "../modules/sysinfo/module.hpp"
#include "../modules/workspaces/hyprland/module.hpp"
#include "../modules/statusnotifier/manager.hpp"
#include "../modules/statusnotifier/module.hpp"

struct Window {
  GtkWidget *window = nullptr;

  SysInfoModule sysinfoModule;
  MprisModule mprisModule;
  HyprWSModule hyprModule;
  ScreenSaverModule scrnsavrModule;
  BluetoothModule btModule;
  NotificationModule notifModule;
  StatusNotifierModule snModule;

  Window(AppContext *ctx, MprisManager *mprisMgr,
         ScreenSaverManager *scrnsavrMgr, NotificationManager *notifInstance,
         BluetoothManager *btMgr, HyprWSManager *hyprMgr, StatusNotifierManager *snManager, Stats* stat, Memory* mem, SysLoad* load, BatteryInfo* battery);
};

class MainWindow {
  GtkApplication *app = nullptr;
  std::vector<std::unique_ptr<Window>> mainWindows;
  AppContext ctx;

  Stats stat;
  Memory mem;
  SysLoad load;
  BatteryInfo battery;
  BluetoothManager btManager;
  NotificationManager notifManager;
  MprisManager mprisManager;
  ScreenSaverManager scrnsavrManager;
  HyprWSManager hyprInstance;
  StatusNotifierManager snManager;
  
  std::thread ssnDBusThread;
  void captureSessionDBus();

public:
  MainWindow();
  void RunApp();
  static gboolean UpdateData(gpointer data);
  static void activate(GtkApplication *app, gpointer user_data);
};
