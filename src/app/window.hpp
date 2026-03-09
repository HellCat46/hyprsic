#pragma once

#include "../modules/bluetooth/module.hpp"
#include "../modules/brightness/module.hpp"
#include "../modules/mpris/module.hpp"
#include "../modules/notifications/module.hpp"
#include "../modules/pulseaudio/module.hpp"
#include "../modules/screensaver/module.hpp"
#include "../modules/statusnotifier/module.hpp"
#include "../modules/sysinfo/module.hpp"
#include "../modules/wifi/module.hpp"
#include "../modules/workspaces/hyprland/module.hpp"
#include "gdk/gdk.h"
#include "modules/sysinfo/manager/temperature.hpp"
#include <gtk-layer-shell.h>
#include <gtk/gtk.h>

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
  WifiModule wifiModule;

  Window(AppContext *ctx, HyprWSManager *hyprMgr,
         StatusNotifierManager *snManager, Stats *stat, Memory *mem,
         SysLoad *load, BatteryInfo *battery, TemperatureManager *tempMgr,
         ScreenSaverManager *scrnsavrMgr, MprisManager *mprisMgr,
         MprisWindow *mprisWindow, NotificationManager *notifInstance,
         NotificationWindow *notifWindow, BluetoothManager *btMgr,
         BluetoothWindow *btWindow, BrightnessManager *brightnessMgr,
         BrightnessWindow *brtWindow, PulseAudioManager *paMgr,
         PulseAudioWindow *paWindow, WifiManager *wifiMgr,
         WifiWindow *wifiWindow);

public:
  void create(GtkApplication *app, GdkDisplay *dp, int i);
  void update();
};
