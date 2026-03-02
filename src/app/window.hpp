#pragma once

#include "context.hpp"
#include "glib.h"
#include <gtk-layer-shell.h>
#include <gtk/gtk.h>
#include <memory>
#include <vector>

#include "../modules/bluetooth/module.hpp"
#include "../modules/brightness/manager.hpp"
#include "../modules/brightness/module.hpp"
#include "../modules/clipboard/manager.hpp"
#include "../modules/mpris/module.hpp"
#include "../modules/notifications/module.hpp"
#include "../modules/pulseaudio/manager.hpp"
#include "../modules/pulseaudio/module.hpp"
#include "../modules/screensaver/module.hpp"
#include "../modules/statusnotifier/manager.hpp"
#include "../modules/statusnotifier/module.hpp"
#include "../modules/sysinfo/module.hpp"
#include "../modules/wifi/manager.hpp"
#include "../modules/workspaces/hyprland/module.hpp"

struct Window {
  GtkWidget *window = nullptr;

  SysInfoModule sysinfoModule;
  MprisModule mprisModule;
  HyprWSModule hyprModule;
  ScreenSaverModule scrnsavrModule;
  BluetoothModule btModule;
  NotificationModule notifModule;
  StatusNotifierModule snModule;
  PulseAudioModule paModule;
  BrightnessModule brtModule;

  Window(AppContext *ctx, MprisManager *mprisMgr,
         ScreenSaverManager *scrnsavrMgr, NotificationManager *notifInstance,
         BluetoothManager *btMgr, HyprWSManager *hyprMgr,
         StatusNotifierManager *snManager, Stats *stat, Memory *mem,
         SysLoad *load, BatteryInfo *battery, PulseAudioManager *paMgr,
         BrightnessManager *brightnessMgr, BluetoothWindow *btWindow,
         BrightnessWindow *brtWindow, MprisWindow *mprisWindow,
         NotificationWindow *notifWindow, PulseAudioWindow *paWindow);
};


class Application {
  GtkApplication *app = nullptr;
  std::vector<std::unique_ptr<Window>> mainWindows;
  AppContext ctx;

  short delay = 5000;

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
  PulseAudioManager paManager;
  WifiManager wifiManager;
  ClipboardManager clipboardManager;
  BrightnessManager brtManager;

  BluetoothWindow btWindow;
  BrightnessWindow brtWindow;
  MprisWindow mprisWindow;
  NotificationWindow notifWindow;
  PulseAudioWindow paWindow;

  std::thread ssnDBusThread, dataUpdateThread, cliIPCThread;
  void captureSessionDBus();
  void UpdateData();
  
  // IPC Handling
  void captureCLIIPC();
  void handleActions(std::string_view action, std::vector<std::string_view> args);
  void handleIPCAction(std::string_view module);
  
  static gboolean UpdateUI(gpointer data);
  
  static void activate(GtkApplication *app, gpointer user_data);

public:
  Application();
  int Run(int argc, char **argv);
};



struct IPCUIData {
    std::string_view action;
    std::vector<std::string_view> args;
    Application *app;
};