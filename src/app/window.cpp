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

MainWindow::MainWindow() : btModule(&ctx), hyprModule(&ctx), notifManager(&ctx) {
  notifManager.RunService();
  load.Init(&ctx.logging);
  mem.Init(&ctx.logging);
  battery.Init(&ctx.logging);
  stat.Init(&ctx.logging);
  playing.Init(&ctx.logging);

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

  GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add(GTK_CONTAINER(self->window), main_box);
  std::string txt = "";


  // Right Box to Show System Stats
  GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  gtk_box_pack_end(GTK_BOX(main_box), right_box, FALSE, FALSE, 5);

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

  gtk_box_pack_start(GTK_BOX(right_box), self->netWid, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(right_box), self->diskWid, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(right_box), self->loadWid, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(right_box), self->memWid, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(right_box), self->batteryWid, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(right_box), self->timeWid, FALSE, FALSE, 0);

  self->hyprModule.setupWorkspaces(main_box);
  self->btModule.setupBT(right_box);

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

  self->btModule.updateBTList();

  return true;
}