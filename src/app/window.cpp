#include "header/window.hpp"
#include <cstring>
#include <ctime>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define TAG "Application"

Window::Window(AppContext *ctx, HyprWSManager *hyprMgr,
               StatusNotifierManager *snManager, Stats *stat, Memory *mem,
               SysLoad *load, BatteryInfo *battery, TemperatureManager *tempMgr,
               ScreenSaverManager *scrnsavrMgr, MprisManager *mprisMgr,
               MprisWindow *mprisWindow, NotificationManager *notifInstance,
               NotificationWindow *notifWindow, BluetoothManager *btMgr,
               BluetoothWindow *btWindow, BrightnessManager *brtMgr,
               BrightnessWindow *brtWindow, PulseAudioManager *paMgr,
               PulseAudioWindow *paWindow, WifiManager *wifiMgr,
               WifiWindow *wifiWin)
    : sysinfoModule(ctx, stat, mem, load, battery, tempMgr),
      mprisModule(ctx, mprisMgr, mprisWindow), hyprModule(ctx, hyprMgr),
      scrnsavrModule(ctx, scrnsavrMgr), btModule(ctx, btMgr, btWindow),
      notifModule(ctx, notifInstance, notifWindow), snModule(ctx, snManager),
      paModule(paMgr, ctx, paWindow), brtModule(ctx, brtMgr, brtWindow),
      wifiModule(ctx, wifiMgr, wifiWin) {}

void Window::create(GtkApplication *app, GdkDisplay *dp, int monitorIdx) {
  auto monitor = gdk_display_get_monitor(dp, monitorIdx);

  window = gtk_application_window_new(app);

  gtk_layer_init_for_window(GTK_WINDOW(window));
  gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_TOP);
  gtk_layer_set_monitor(GTK_WINDOW(window), monitor);

  gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, true);
  gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, true);
  gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, true);
  gtk_layer_set_exclusive_zone(GTK_WINDOW(window), 25);
  gtk_widget_set_size_request(window, -1, 25);

  GtkWidget *mainGrid = gtk_grid_new();
  gtk_grid_set_column_homogeneous(GTK_GRID(mainGrid), true);
  gtk_container_add(GTK_CONTAINER(window), mainGrid);

  GtkWidget *wid = hyprModule.setup(monitorIdx);
  gtk_grid_attach(GTK_GRID(mainGrid), wid, 0, 0, 2, 1);

  wid = mprisModule.setup();
  gtk_grid_attach(GTK_GRID(mainGrid), wid, 2, 0, 1, 1);

  // Right Box to Show System Stats
  GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach(GTK_GRID(mainGrid), right_box, 3, 0, 3, 1);
  gtk_widget_set_hexpand(right_box, true);

  GtkWidget *rightGrid = gtk_grid_new();
  gtk_box_pack_end(GTK_BOX(right_box), rightGrid, false, false, 0);
  gtk_grid_set_column_spacing(GTK_GRID(rightGrid), 10);
  gtk_widget_set_margin_end(right_box, 5);

  // System Info Widgets
  auto wids = sysinfoModule.setup();
  for (unsigned long i = 0; i < wids.size(); i++) {
    gtk_grid_attach(GTK_GRID(rightGrid), wids[i], i, 0, 1, 1);
  }

  int loc = wids.size();
  wid = wifiModule.setup();
  gtk_grid_attach(GTK_GRID(rightGrid), wid, loc++, 0, 1, 1);

  wid = brtModule.setup();
  gtk_grid_attach(GTK_GRID(rightGrid), wid, loc++, 0, 1, 1);

  wid = notifModule.setup();
  gtk_grid_attach(GTK_GRID(rightGrid), wid, loc++, 0, 1, 1);

  wid = btModule.setup();
  gtk_grid_attach(GTK_GRID(rightGrid), wid, loc++, 0, 1, 1);

  wid = scrnsavrModule.setup();
  gtk_grid_attach(GTK_GRID(rightGrid), wid, loc++, 0, 1, 1);

  wids = paModule.setup();
  for (unsigned long i = 0; i < wids.size(); i++) {
    gtk_grid_attach(GTK_GRID(rightGrid), wids[i], loc + i, 0, 1, 1);
  }

  loc += wids.size();
  wid = snModule.setup();
  gtk_grid_attach(GTK_GRID(rightGrid), wid, loc, 0, 1, 1);

  gtk_widget_show_all(window);
}

void Window::update() {
  sysinfoModule.update();
  mprisModule.update();
  snModule.update();
  paModule.update();
  brtModule.update();
  wifiModule.update();
}
