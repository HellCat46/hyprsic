#include "window.hpp"
#include "gdk/gdk.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk-layer-shell.h"
#include "gtk/gtk.h"
#include <ctime>
#include <iomanip>
#include <memory>
#include <string>

#define TAG "MainWindow"

Window::Window(AppContext *ctx, MprisManager *mprisMgr,
               ScreenSaverManager *scrnsavrMgr,
               NotificationManager *notifInstance, BluetoothManager *btMgr,
               HyprWSManager *hyprMgr)
    : mprisModule(ctx, mprisMgr), hyprModule(ctx, hyprMgr),
      scrnsavrModule(ctx, scrnsavrMgr), btModule(ctx, btMgr),
      notifModule(ctx, notifInstance) {}

MainWindow::MainWindow()
    : notifManager(&ctx), btManager(&ctx), mprisManager(&ctx),
      scrnsavrManager(&ctx), hyprInstance(&ctx.logging) {
  load.Init(&ctx.logging);
  mem.Init(&ctx.logging);
  battery.Init(&ctx.logging);
  stat.Init(&ctx.logging);
  btManager.setup();
  // playing.Init(&ctx.logging);

  hyprInstance.liveEventListener();
  btManager.getDeviceList();
  notifManager.RunService(NotificationModule::showNotification);

  app = gtk_application_new("com.hellcat.hyprsic", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), this);
  g_timeout_add(5000, UpdateData, this);
}

void MainWindow::RunApp() {
  int status = g_application_run(G_APPLICATION(app), 0, nullptr);
  g_object_unref(app);
}

void MainWindow::activate(GtkApplication *app, gpointer user_data) {
  MainWindow *self = static_cast<MainWindow *>(user_data);
  GdkDisplay *display = gdk_display_get_default();
  int mCount = gdk_display_get_n_monitors(display);

  for (int i = 0; i < mCount; i++) {
    std::unique_ptr<Window> winInstance = std::unique_ptr<Window>(new Window(&self->ctx, &self->mprisManager, &self->scrnsavrManager,
                       &self->notifManager, &self->btManager,
                       &self->hyprInstance));
    GdkMonitor *monitor = gdk_display_get_monitor(display, i);

    winInstance->window = gtk_application_window_new(app);

    gtk_layer_init_for_window(GTK_WINDOW(winInstance->window));
    gtk_layer_set_layer(GTK_WINDOW(winInstance->window),
                        GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_monitor(GTK_WINDOW(winInstance->window), monitor);

    gtk_layer_set_anchor(GTK_WINDOW(winInstance->window),
                         GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(winInstance->window),
                         GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(winInstance->window),
                         GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_exclusive_zone(GTK_WINDOW(winInstance->window), 25);
    gtk_widget_set_size_request(winInstance->window, -1, 25);

    GtkWidget *main_box = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(main_box), TRUE);
    gtk_container_add(GTK_CONTAINER(winInstance->window), main_box);
    std::string txt = "";

    // Right Box to Show System Stats
    GtkWidget *right_box = gtk_grid_new();

    gtk_grid_attach(GTK_GRID(main_box), right_box, 3, 0, 2, 1);
    gtk_widget_set_hexpand(right_box, TRUE);
    gtk_grid_set_column_spacing(GTK_GRID(right_box), 2);

    txt = "⬇" + self->stat.GetNetRx() + "⬆" + self->stat.GetNetTx();
    winInstance->netWid = gtk_label_new(txt.c_str());

    txt = " " + self->stat.GetDiskAvail() + "/" + self->stat.GetDiskTotal();
    winInstance->diskWid = gtk_label_new(txt.c_str());

    self->stat.UpdateData();

    txt = std::to_string(self->load.GetLoad(5));
    txt = " " + txt.substr(0, txt.find('.') + 3);
    winInstance->loadWid = gtk_label_new(txt.c_str());

    txt = " " + Stats::ParseBytes(self->mem.GetUsedRAM() * 1000, 2) + "/" +
          Stats::ParseBytes(self->mem.GetTotRAM() * 1000, 2);
    winInstance->memWid = gtk_label_new(txt.c_str());

    txt = " " + std::to_string(self->battery.getTotPercent()) + "%";
    winInstance->batteryWid = gtk_label_new(txt.c_str());

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    winInstance->timeWid = gtk_label_new(oss.str().c_str());

    gtk_grid_attach(GTK_GRID(right_box), winInstance->netWid, 0, 0, 1, 1);
    gtk_widget_set_hexpand(winInstance->netWid, TRUE);
    gtk_grid_attach(GTK_GRID(right_box), winInstance->diskWid, 1, 0, 1, 1);
    gtk_widget_set_hexpand(winInstance->diskWid, TRUE);
    gtk_grid_attach(GTK_GRID(right_box), winInstance->loadWid, 2, 0, 1, 1);
    gtk_widget_set_hexpand(winInstance->loadWid, TRUE);
    gtk_grid_attach(GTK_GRID(right_box), winInstance->memWid, 3, 0, 1, 1);
    gtk_widget_set_hexpand(winInstance->memWid, TRUE);
    gtk_grid_attach(GTK_GRID(right_box), winInstance->batteryWid, 4, 0, 1, 1);
    gtk_widget_set_hexpand(winInstance->batteryWid, TRUE);
    gtk_grid_attach(GTK_GRID(right_box), winInstance->timeWid, 5, 0, 1, 1);
    gtk_widget_set_hexpand(winInstance->timeWid, TRUE);

    winInstance->mprisModule.setup(main_box);
    winInstance->hyprModule.setup(main_box);

    winInstance->notifModule.setup(right_box);

    winInstance->btModule.setupBT(right_box);
    winInstance->scrnsavrModule.setup(right_box);

    
    gtk_widget_show_all(winInstance->window);
    self->mainWindows.push_back(std::move(winInstance));
  }
}

gboolean MainWindow::UpdateData(gpointer data) {
  MainWindow *self = static_cast<MainWindow *>(data);
  std::string txt = "";

  self->btManager.getDeviceList();
  for (auto &window : self->mainWindows) {

    // Update Network Usage
    txt = "⬇" + self->stat.GetNetRx() + "⬆" + self->stat.GetNetTx();
    gtk_label_set_label(GTK_LABEL(window->netWid), txt.c_str());

    // Update Disk Usage
    txt = " " + self->stat.GetDiskAvail() + "/" + self->stat.GetDiskTotal();
    gtk_label_set_label(GTK_LABEL(window->diskWid), txt.c_str());

    // Update System Load
    txt = std::to_string(self->load.GetLoad(5));
    txt = " " + txt.substr(0, txt.find('.') + 3);
    gtk_label_set_label(GTK_LABEL(window->loadWid), txt.c_str());

    // Update memory
    txt = " " + Stats::ParseBytes(self->mem.GetUsedRAM() * 1000, 2) + "/" +
          Stats::ParseBytes(self->mem.GetTotRAM() * 1000, 2);
    gtk_label_set_label(GTK_LABEL(window->memWid), txt.c_str());

    // Update battery
    txt = " " + std::to_string(self->battery.getTotPercent()) + "%";
    gtk_label_set_label(GTK_LABEL(window->batteryWid),
                        txt.c_str());

    // Update time
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    gtk_label_set_label(GTK_LABEL(window->timeWid),
                        oss.str().c_str());

    window->mprisModule.update();
    window->notifModule.update();
    window->btModule.updateBTList();
  }

  self->stat.UpdateData();

  return true;
}
