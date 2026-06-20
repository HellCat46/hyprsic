#pragma once

#include "glibmm/refptr.h"
#include "modules/bluetooth/header/window.hpp"
#include "modules/mpris/header/window.hpp"
#include "modules/pulseaudio/header/window.hpp"
#include "modules/wifi/header/window.hpp"
#include "services/header/comm_bus.hpp"
#include "services/header/context.hpp"
#include "window.hpp"
#include <memory>
#include <vector>
class Application : public Gtk::Application {
  std::vector<std::unique_ptr<AppWindow>> mainWindows;
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

  CommunicationBus commBus;

  // Threads for DBus and IPC
  std::thread dataUpdateThread, cliIPCThread;
  void captureSessionDBus();
  void captureSystemDBus();
  void UpdateData();
  bool UpdateUI();

  // IPC Handling
  void captureCLIIPC();
  void handleActions(std::string_view action,
                     std::vector<std::string_view> args);
  void IPCToggleView(std::string_view module);
  void IPCCtrlAudioDev(std::string_view action);

protected:
  void on_activate() override;

public:
  Application();
  static Glib::RefPtr<Application> create();
  ~Application();
};

struct IPCUIData {
  std::string_view action;
  std::vector<std::string_view> args;
  Application *app;
};
