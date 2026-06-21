#include "header/window.hpp"
#include "gtk4-layer-shell.h"
#include "gtkmm-4.0/gdkmm/monitor.h"
#include "gtkmm/label.h"
#include "services/header/comm_bus.hpp"
#include <functional>
#include <memory>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

#define TAG "Application"

AppWindow::AppWindow(AppContext *ctx, CommunicationBus *commBus,
                     HyprWSManager *hyprMgr, StatusNotifierManager *snManager,
                     Stats *stat, Memory *mem, SysLoad *load,
                     BatteryInfo *battery, TemperatureManager *tempMgr,
                     ScreenSaverManager *scrnsavrMgr, MprisManager *mprisMgr,
                     NotificationManager *notifInstance,
                    BluetoothManager *btMgr,
                     BrightnessManager *brtMgr,
                     PulseAudioManager *paMgr, WifiManager *wifiMgr)
    : sysinfoModule(ctx, stat, mem, load, battery, tempMgr),
      mprisModule(ctx, mprisMgr, commBus),
      hyprModule(ctx, commBus, hyprMgr),
      scrnsavrModule(ctx, commBus, scrnsavrMgr), btModule(ctx, btMgr),
      notifModule(ctx, commBus, notifInstance),
      snModule(ctx, commBus, snManager), paModule(paMgr, ctx),
      brtModule(ctx, brtMgr), wifiModule(ctx, wifiMgr) {}

void AppWindow::create(std::shared_ptr<Gdk::Monitor> monitor, int monIdx) {

  auto winObj = this->gobj();
  gtk_layer_init_for_window(winObj);
  gtk_layer_set_layer(winObj, GTK_LAYER_SHELL_LAYER_TOP);
  gtk_layer_set_monitor(winObj, monitor->gobj());
  gtk_layer_set_anchor(winObj, GTK_LAYER_SHELL_EDGE_BOTTOM, true);
  gtk_layer_set_anchor(winObj, GTK_LAYER_SHELL_EDGE_LEFT, true);
  gtk_layer_set_anchor(winObj, GTK_LAYER_SHELL_EDGE_RIGHT, true);
  gtk_layer_set_exclusive_zone(winObj, 25);

  set_size_request(-1, 25);

  Gtk::Box mainBox;
  set_child(mainBox);

  mainBox.append(hyprModule.setup(monIdx));

  auto& mpris = mprisModule.setup();
  mpris.set_hexpand(true);
  mpris.set_halign(Gtk::Align::CENTER);
  mainBox.append(mpris);

  Gtk::Box rightBox;
  rightBox.set_spacing(10);
  rightBox.set_halign(Gtk::Align::END);
  mainBox.append(rightBox);

  // System Info Widgets
  std::vector<std::reference_wrapper<Gtk::Label>> wids;
  sysinfoModule.setup(wids);
  for (auto& w : wids)
      rightBox.append(w.get());
  
  rightBox.append(wifiModule.setup());
  rightBox.append(brtModule.setup());
  rightBox.append(notifModule.setup());
  rightBox.append(btModule.setup());
  rightBox.append(scrnsavrModule.setup());
  rightBox.append(paModule.setup());
  rightBox.append(snModule.setup());

  this->show();
}

void AppWindow::update() {
  sysinfoModule.update();
  mprisModule.update();
  snModule.update();
  paModule.update();
  brtModule.update();
  wifiModule.update();
}
