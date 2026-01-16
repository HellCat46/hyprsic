#include "window.hpp"
#include "gdk/gdk.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "glibconfig.h"
#include "gtk-layer-shell.h"
#include "gtk/gtk.h"
#include <ctime>
#include <iomanip>
#include <string>

#define TAG "MainWindow"

MainWindow::MainWindow()
    : btModule(&ctx), notifModule(&ctx), mprisManager(&ctx),
      screenSaverModule(&ctx) {
  load.Init(&ctx.logging);
  mem.Init(&ctx.logging);
  battery.Init(&ctx.logging);
  stat.Init(&ctx.logging);
  // playing.Init(&ctx.logging);

  app =
      gtk_application_new("com.hyprsic.statusbar", G_APPLICATION_DEFAULT_FLAGS);
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
    Window winInstance;
    GdkMonitor *monitor = gdk_display_get_monitor(display, i);

    winInstance.window = gtk_application_window_new(app);

    gtk_layer_init_for_window(GTK_WINDOW(winInstance.window));
    gtk_layer_set_layer(GTK_WINDOW(winInstance.window),
                        GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_monitor(GTK_WINDOW(winInstance.window), monitor);

    gtk_layer_set_anchor(GTK_WINDOW(winInstance.window),
                         GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(winInstance.window),
                         GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(winInstance.window),
                         GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_exclusive_zone(GTK_WINDOW(winInstance.window), 25);
    gtk_widget_set_size_request(winInstance.window, -1, 25);

    GtkWidget *main_box = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(main_box), TRUE);
    gtk_container_add(GTK_CONTAINER(winInstance.window), main_box);
    std::string txt = "";

    // Right Box to Show System Stats
    GtkWidget *right_box = gtk_grid_new();

    gtk_grid_attach(GTK_GRID(main_box), right_box, 3, 0, 2, 1);
    gtk_widget_set_hexpand(right_box, TRUE);
    gtk_grid_set_column_spacing(GTK_GRID(right_box), 2);

    txt = "⬇" + self->stat.GetNetRx() + "⬆" + self->stat.GetNetTx();
    winInstance.netWid = gtk_label_new(txt.c_str());

    txt = " " + self->stat.GetDiskAvail() + "/" + self->stat.GetDiskTotal();
    winInstance.diskWid = gtk_label_new(txt.c_str());

    self->stat.UpdateData();

    txt = std::to_string(self->load.GetLoad(5));
    txt = " " + txt.substr(0, txt.find('.') + 3);
    winInstance.loadWid = gtk_label_new(txt.c_str());

    txt = " " + Stats::ParseBytes(self->mem.GetUsedRAM() * 1000, 2) + "/" +
          Stats::ParseBytes(self->mem.GetTotRAM() * 1000, 2);
    winInstance.memWid = gtk_label_new(txt.c_str());

    txt = " " + std::to_string(self->battery.getTotPercent()) + "%";
    winInstance.batteryWid = gtk_label_new(txt.c_str());

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    winInstance.timeWid = gtk_label_new(oss.str().c_str());

    gtk_grid_attach(GTK_GRID(right_box), winInstance.netWid, 0, 0, 1, 1);
    gtk_widget_set_hexpand(winInstance.netWid, TRUE);
    gtk_grid_attach(GTK_GRID(right_box), winInstance.diskWid, 1, 0, 1, 1);
    gtk_widget_set_hexpand(winInstance.diskWid, TRUE);
    gtk_grid_attach(GTK_GRID(right_box), winInstance.loadWid, 2, 0, 1, 1);
    gtk_widget_set_hexpand(winInstance.loadWid, TRUE);
    gtk_grid_attach(GTK_GRID(right_box), winInstance.memWid, 3, 0, 1, 1);
    gtk_widget_set_hexpand(winInstance.memWid, TRUE);
    gtk_grid_attach(GTK_GRID(right_box), winInstance.batteryWid, 4, 0, 1, 1);
    gtk_widget_set_hexpand(winInstance.batteryWid, TRUE);
    gtk_grid_attach(GTK_GRID(right_box), winInstance.timeWid, 5, 0, 1, 1);
    gtk_widget_set_hexpand(winInstance.timeWid, TRUE);

    winInstance.mprisModule = new MprisModule(&self->ctx, &self->mprisManager);
    winInstance.mprisModule->setup(main_box);
    
    winInstance.hyprModule = new HyprWSModule(&self->ctx);  
    winInstance.hyprModule->setup(main_box);

    self->mainWindows.push_back(winInstance);

    gtk_widget_show_all(winInstance.window);
  }

  // self->notifModule.setup(right_box);
  // self->btModule.setupBT(right_box);
  // self->screenSaverModule.setup(right_box);

  // gtk_widget_show_all(self->window);
}

gboolean MainWindow::UpdateData(gpointer data) {
  MainWindow *self = static_cast<MainWindow *>(data);
  std::string txt = "";

  for (auto winInstance : self->mainWindows) {
    self->stat.UpdateData();

    // Update Network Usage
    txt = "⬇" + self->stat.GetNetRx() + "⬆" + self->stat.GetNetTx();
    gtk_label_set_label(GTK_LABEL(winInstance.netWid), txt.c_str());

    // Update Disk Usage
    txt = " " + self->stat.GetDiskAvail() + "/" + self->stat.GetDiskTotal();
    gtk_label_set_label(GTK_LABEL(winInstance.diskWid), txt.c_str());

    // Update System Load
    txt = std::to_string(self->load.GetLoad(5));
    txt = " " + txt.substr(0, txt.find('.') + 3);
    gtk_label_set_label(GTK_LABEL(winInstance.loadWid), txt.c_str());

    // Update memory
    txt = " " + Stats::ParseBytes(self->mem.GetUsedRAM() * 1000, 2) + "/" +
          Stats::ParseBytes(self->mem.GetTotRAM() * 1000, 2);
    gtk_label_set_label(GTK_LABEL(winInstance.memWid), txt.c_str());

    // Update battery
    txt = " " + std::to_string(self->battery.getTotPercent()) + "%";
    gtk_label_set_label(GTK_LABEL(winInstance.batteryWid), txt.c_str());

    // Update time
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    gtk_label_set_label(GTK_LABEL(winInstance.timeWid), oss.str().c_str());

    winInstance.mprisModule->Update();
    // self->notifModule.update();
    // self->btModule.updateBTList();
  }

  

  return true;
}
