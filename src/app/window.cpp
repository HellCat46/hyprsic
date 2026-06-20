#include "header/window.hpp"
#include "gtk4-layer-shell.h"
#include "gtkmm-4.0/gdkmm/monitor.h"
#include "gtkmm/grid.h"
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

  Gtk::Grid mainGrid;
  mainGrid.set_column_homogeneous(true);
  set_child(mainGrid);

  mainGrid.attach(hyprModule.setup(monIdx), 0, 0, 2, 1);
  mainGrid.attach(mprisModule.setup(), 2, 0, 1, 1);

  Gtk::Box rightBox;
  rightBox.set_spacing(0);
  rightBox.set_hexpand(true);
  mainGrid.attach(rightBox, 3, 0, 3, 1);

  Gtk::Grid rightGrid;
  rightBox.insert_at_end(rightGrid);
  rightGrid.set_hexpand(true);
  rightGrid.set_halign(Gtk::Align::FILL);
  rightBox.set_halign(Gtk::Align::FILL);
  rightGrid.set_column_spacing(10);
  rightBox.set_margin_end(5);

  // System Info Widgets
  std::vector<std::reference_wrapper<Gtk::Label>> wids;
  sysinfoModule.setup(wids);
  for (unsigned long i = 0; i < wids.size(); i++) {
    rightGrid.attach(wids[i], i, 0, 1, 1);
  }

  int loc = wids.size();

  rightGrid.attach(wifiModule.setup(), loc++, 0, 1, 1);
  rightGrid.attach(brtModule.setup(), loc++, 0, 1, 1);
  rightGrid.attach(notifModule.setup(), loc++, 0, 1, 1);
  rightGrid.attach(btModule.setup(), loc++, 0, 1, 1);
  rightGrid.attach(scrnsavrModule.setup(), loc++, 0, 1, 1);

  rightGrid.attach(paModule.setup(), loc, 0, 2, 1);
  loc += 2;

  rightGrid.attach(snModule.setup(), loc, 0, 1, 1);
  loc++;

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
