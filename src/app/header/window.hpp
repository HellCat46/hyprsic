#pragma once

#include "gdkmm/monitor.h"
#include "modules/bluetooth/header/module.hpp"
#include "modules/brightness/header/module.hpp"
#include "modules/mpris/header/module.hpp"
#include "modules/notifications/header/module.hpp"
#include "modules/pulseaudio/header/module.hpp"
#include "modules/screensaver/header/module.hpp"
#include "modules/statusnotifier/header/module.hpp"
#include "modules/sysinfo/header/manager/temperature.hpp"
#include "modules/sysinfo/header/module.hpp"
#include "modules/wifi/header/module.hpp"
#include "modules/workspaces/hyprland/header/module.hpp"
#include <gtkmm-4.0/gtkmm.h>
#include <memory>

struct AppWindow : public Gtk::Window {

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

  AppWindow(AppContext *ctx, CommunicationBus *commBus, HyprWSManager *hyprMgr,
            StatusNotifierManager *snManager, Stats *stat, Memory *mem,
            SysLoad *load, BatteryInfo *battery, TemperatureManager *tempMgr,
            ScreenSaverManager *scrnsavrMgr, MprisManager *mprisMgr,
            NotificationManager *notifInstance, BluetoothManager *btMgr,
            BrightnessManager *brightnessMgr, PulseAudioManager *paMgr,
            WifiManager *wifiMgr);

public:
  void create(std::shared_ptr<Gdk::Monitor> monitor, int monIdx);
  void update();
};
