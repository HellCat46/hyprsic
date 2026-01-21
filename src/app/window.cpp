#include "window.hpp"
#include "gdk/gdk.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk-layer-shell.h"
#include "gtk/gtk.h"
#include <ctime>
#include <memory>

#define TAG "MainWindow"

Window::Window(AppContext *ctx, MprisManager *mprisMgr,
               ScreenSaverManager *scrnsavrMgr,
               NotificationManager *notifInstance, BluetoothManager *btMgr,
               HyprWSManager *hyprMgr, StatusNotifierManager *snManager,
               Stats *stat, Memory *mem, SysLoad *load, BatteryInfo *battery)
    : mprisModule(ctx, mprisMgr), hyprModule(ctx, hyprMgr),
      scrnsavrModule(ctx, scrnsavrMgr), btModule(ctx, btMgr),
      notifModule(ctx, notifInstance), snModule(ctx, snManager),
      sysinfoModule(stat, mem, load, battery) {}

MainWindow::MainWindow()
    : notifManager(&ctx), btManager(&ctx), mprisManager(&ctx),
      scrnsavrManager(&ctx), hyprInstance(&ctx.logging), snManager(&ctx) {
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
    std::unique_ptr<Window> winInstance = std::unique_ptr<Window>(
        new Window(&self->ctx, &self->mprisManager, &self->scrnsavrManager,
                   &self->notifManager, &self->btManager, &self->hyprInstance,
                   &self->snManager, &self->stat, &self->mem, &self->load,
                   &self->battery));
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

    // Right Box to Show System Stats
    GtkWidget *right_box = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(main_box), right_box, 3, 0, 2, 1);
    gtk_widget_set_hexpand(right_box, TRUE);
    gtk_grid_set_column_spacing(GTK_GRID(right_box), 2);

    
    winInstance->hyprModule.setup(main_box);
    
    winInstance->mprisModule.setup(main_box);
    
    winInstance->sysinfoModule.setup(right_box);
    winInstance->notifModule.setup(right_box);
    winInstance->btModule.setupBT(right_box);
    winInstance->scrnsavrModule.setup(right_box);
    winInstance->snModule.setup(right_box);

    gtk_widget_show_all(winInstance->window);
    self->mainWindows.push_back(std::move(winInstance));
  }
}

gboolean MainWindow::UpdateData(gpointer data) {
  MainWindow *self = static_cast<MainWindow *>(data);

  self->btManager.getDeviceList();
  for (auto &window : self->mainWindows) {
    window->sysinfoModule.update();
    window->mprisModule.update();
    window->notifModule.update();
    window->btModule.updateBTList();
    window->snModule.update();
  }
  self->stat.UpdateData();

  return true;
}
