#include "window.hpp"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "glibconfig.h"
#include "gtk/gtk.h"
#include <ctime>
#include <iomanip>
#include <string>

#define TAG "MainWindow"

MainWindow::MainWindow() : btModule(&ctx), hyprModule(&ctx), notifModule(&ctx), mprisModule(&ctx), screenSaverModule(&ctx) {
  load.Init(&ctx.logging);
  mem.Init(&ctx.logging);
  battery.Init(&ctx.logging);
  stat.Init(&ctx.logging);
  //playing.Init(&ctx.logging);

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
  self->window = gtk_application_window_new(app);

  gtk_layer_init_for_window(GTK_WINDOW(self->window));

  gtk_layer_set_layer(GTK_WINDOW(self->window), GTK_LAYER_SHELL_LAYER_TOP);
  gtk_layer_set_anchor(GTK_WINDOW(self->window), GTK_LAYER_SHELL_EDGE_BOTTOM,
                       TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(self->window), GTK_LAYER_SHELL_EDGE_LEFT,
                       TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(self->window), GTK_LAYER_SHELL_EDGE_RIGHT,
                       TRUE);
  gtk_layer_set_exclusive_zone(GTK_WINDOW(self->window), 35);

  GtkWidget *main_box = gtk_grid_new();
  gtk_grid_set_column_homogeneous(GTK_GRID(main_box), TRUE);
  gtk_container_add(GTK_CONTAINER(self->window), main_box);
  std::string txt = "";

  // Right Box to Show System Stats
  GtkWidget *right_box = gtk_grid_new();

  gtk_grid_attach(GTK_GRID(main_box), right_box, 3, 0, 2, 1);
  gtk_widget_set_hexpand(right_box, TRUE);
  gtk_grid_set_column_spacing(GTK_GRID(right_box), 2);

  txt = "⬇" + self->stat.GetNetRx() + "⬆" + self->stat.GetNetTx();
  self->netWid = gtk_label_new(txt.c_str());

  txt = " " + self->stat.GetDiskAvail() + "/" + self->stat.GetDiskTotal();
  self->diskWid = gtk_label_new(txt.c_str());

  self->stat.UpdateData();

  txt = std::to_string(self->load.GetLoad(5));
  txt = " " + txt.substr(0, txt.find('.') + 3);
  self->loadWid = gtk_label_new(txt.c_str());

  txt = " " + Stats::ParseBytes(self->mem.GetUsedRAM() * 1000, 2) + "/" +
        Stats::ParseBytes(self->mem.GetTotRAM() * 1000, 2);
  self->memWid = gtk_label_new(txt.c_str());

  txt = " " + std::to_string(self->battery.getTotPercent()) + "%";
  self->batteryWid = gtk_label_new(txt.c_str());

  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%H:%M:%S");
  self->timeWid = gtk_label_new(oss.str().c_str());

  gtk_grid_attach(GTK_GRID(right_box), self->netWid, 0, 0, 1, 1);
  gtk_widget_set_hexpand(self->netWid, TRUE);
  gtk_grid_attach(GTK_GRID(right_box), self->diskWid, 1, 0, 1, 1);
  gtk_widget_set_hexpand(self->diskWid, TRUE);
  gtk_grid_attach(GTK_GRID(right_box), self->loadWid, 2, 0, 1, 1);
  gtk_widget_set_hexpand(self->loadWid, TRUE);
  gtk_grid_attach(GTK_GRID(right_box), self->memWid, 3, 0, 1, 1);
  gtk_widget_set_hexpand(self->memWid, TRUE);
  gtk_grid_attach(GTK_GRID(right_box), self->batteryWid, 4, 0, 1, 1);
  gtk_widget_set_hexpand(self->batteryWid, TRUE);
  gtk_grid_attach(GTK_GRID(right_box), self->timeWid, 5, 0, 1, 1);
  gtk_widget_set_hexpand(self->timeWid, TRUE);
  
  
  
  self->mprisModule.setup(main_box);
  self->hyprModule.setup(main_box);
  
  self->notifModule.setup(right_box);
  self->btModule.setupBT(right_box);
  self->screenSaverModule.setup(right_box);

  gtk_widget_show_all(self->window);
}

gboolean MainWindow::UpdateData(gpointer data) {
  MainWindow *self = static_cast<MainWindow *>(data);
  std::string txt = "";

  // Update Network Usage
  txt = "⬇" + self->stat.GetNetRx() + "⬆" + self->stat.GetNetTx();
  gtk_label_set_label(GTK_LABEL(self->netWid), txt.c_str());

  // Update Disk Usage
  txt = " " + self->stat.GetDiskAvail() + "/" + self->stat.GetDiskTotal();
  gtk_label_set_label(GTK_LABEL(self->diskWid), txt.c_str());

  // Update System Load
  txt = std::to_string(self->load.GetLoad(5));
  txt = " " + txt.substr(0, txt.find('.') + 3);
  gtk_label_set_label(GTK_LABEL(self->loadWid), txt.c_str());

  // Update memory
  txt = " " + Stats::ParseBytes(self->mem.GetUsedRAM() * 1000, 2) + "/" +
        Stats::ParseBytes(self->mem.GetTotRAM() * 1000, 2);
  gtk_label_set_label(GTK_LABEL(self->memWid), txt.c_str());

  // Update battery
  txt = " " + std::to_string(self->battery.getTotPercent()) + "%";
  gtk_label_set_label(GTK_LABEL(self->batteryWid), txt.c_str());

  // Update time
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%H:%M:%S");
  gtk_label_set_label(GTK_LABEL(self->timeWid), oss.str().c_str());

  self->stat.UpdateData();
  
  self->mprisModule.Update();
 
  self->notifModule.update();
  self->btModule.updateBTList();

  return true;
}