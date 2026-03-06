#pragma once

#include "services/header/context.hpp"
#include "app/window.hpp"
#include "gtk/gtk.h"
#include <memory>
#include <vector>
class Application {
  GtkApplication *app = nullptr;
  std::vector<std::unique_ptr<Window>> mainWindows;
  AppContext ctx;

  short delay = 5000;

  Stats stat;
  Memory mem;
  SysLoad load;
  BatteryInfo battery;
  TemperatureManager tempManager;

  BluetoothManager btManager;
  BluetoothWindow btWindow;

  NotificationManager notifManager;
  NotificationWindow notifWindow;

  MprisManager mprisManager;
  MprisWindow mprisWindow;

  ScreenSaverManager scrnsavrManager;
  HyprWSManager hyprInstance;
  StatusNotifierManager snManager;

  PulseAudioManager paManager;
  PulseAudioWindow paWindow;

  WifiManager wifiManager;
  WifiWindow wifiWindow;

  BrightnessManager brtManager;
  BrightnessWindow brtWindow;
  
  // Threads for DBus and IPC
  std::thread ssnDBusThread, sysDBusThread, dataUpdateThread, cliIPCThread;
  void captureSessionDBus();
  void captureSystemDBus();
  void UpdateData();

  // IPC Handling
  void captureCLIIPC();
  void handleActions(std::string_view action,
                     std::vector<std::string_view> args);
  void IPCToggleView(std::string_view module);
  void IPCCtrlAudioDev(std::string_view action);

  
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
