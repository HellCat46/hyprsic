#include "window.hpp"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "glibconfig.h"
#include "gtk/gtk.h"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>

typedef struct {
  GtkWidget *time_label;
  GtkWidget *date_label;
} AppData;

MainWindow::MainWindow() : btInfo(&ctx) {
  load.Init();
  mem.Init();
  battery.Init();
  hyprWS.Init();

  app =
      gtk_application_new("com.hyprsic.statusbar", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), this);
  g_timeout_add(5000, UpdateData, this);
}

void MainWindow::RunApp() {
  int status = g_application_run(G_APPLICATION(app), 0, NULL);
  g_object_unref(app);
}

void MainWindow::activate(GtkApplication *app, gpointer user_data) {
  MainWindow *self = static_cast<MainWindow *>(user_data);

  GtkWidget *window = gtk_application_window_new(app);

  gtk_layer_init_for_window(GTK_WINDOW(window));

  gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_TOP);
  gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
  gtk_layer_set_exclusive_zone(GTK_WINDOW(window), 30);

  GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add(GTK_CONTAINER(window), main_box);
  std::string txt = "";

  // Left Box to Show Workspaces Info
  self->workspaceSecWid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  gtk_box_pack_start(GTK_BOX(main_box), self->workspaceSecWid, FALSE, FALSE,
                     5);

  // Update Workspaces Info
  if (!self->hyprWS.GetWorkspaces()) {
    for (auto workspace : self->hyprWS.workspaces) {
      txt = std::to_string(workspace.first) + " ";

      if (self->hyprWS.GetActiveWorkspace() == workspace.first) {
        txt = "[ " + txt + "]";
      }

      GtkWidget *wsLabel = gtk_label_new(txt.c_str());
      gtk_box_pack_start(GTK_BOX(self->workspaceSecWid), wsLabel, FALSE, FALSE,
                         0);
    }
  }

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

  setupBT(right_box, self);

  gtk_widget_show_all(window);
}

gboolean MainWindow::UpdateData(gpointer data) {
  MainWindow *self = static_cast<MainWindow *>(data);
  std::string txt = "";

  if (!self->hyprWS.GetWorkspaces()) {
    GList *child =
        gtk_container_get_children(GTK_CONTAINER(self->workspaceSecWid));
    for (GList *iter = child; iter != NULL; iter = iter->next) {
      gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(child);

    for (auto workspace : self->hyprWS.workspaces) {
      txt = std::to_string(workspace.first) + " ";

      if (self->hyprWS.GetActiveWorkspace() == workspace.first) {
        txt = "[ " + txt + "]";
      }

      GtkWidget *wsLabel = gtk_label_new(txt.c_str());
      gtk_box_pack_start(GTK_BOX(self->workspaceSecWid), wsLabel, FALSE, FALSE,
                         0);
    }
    gtk_widget_show_all(self->workspaceSecWid);
  }

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

  return TRUE;
}

void MainWindow::showBTMenu(GtkWidget *widget, gpointer user_data) {
  MainWindow *self = static_cast<MainWindow *>(user_data);

  createBTList(self);

  gtk_popover_popup(GTK_POPOVER(self->btPopOverMenu));
}

void MainWindow::createBTList(MainWindow *self) {

  GList *children = gtk_container_get_children(GTK_CONTAINER(self->btDevList));
  for (GList *iter = children; iter != NULL; iter = iter->next) {
    gtk_widget_destroy(GTK_WIDGET(iter->data));
  }
  g_list_free(children);

  self->btInfo.getDeviceList();

  // Connected Devices Section
  if (self->btInfo.connectedDev.size() > 0) {
    GtkWidget *connDevtitle = gtk_label_new("Connnected Devices: ");
    gtk_box_pack_start(GTK_BOX(self->btDevList), connDevtitle, FALSE, FALSE, 0);
    gtk_widget_show(connDevtitle);

    std::cout << "Bluetooth Devices Found: " << self->btInfo.connectedDev.size()
              << std::endl;
    for (auto [_, device] : self->btInfo.connectedDev) {
      std::string devLabel =
          device.name.length() > 0 ? device.name : device.addr;

      GtkWidget *deviceWid = gtk_button_new_with_label(devLabel.c_str());
      gtk_box_pack_start(GTK_BOX(self->btDevList), deviceWid, FALSE, FALSE, 2);
      gtk_widget_show(deviceWid);
    }
  }

  // Available Devices Section
  if (self->btInfo.availDev.size() > 0) {
    GtkWidget *availDevtitle = gtk_label_new("Available Devices: ");
    gtk_box_pack_start(GTK_BOX(self->btDevList), availDevtitle, FALSE, FALSE, 0);
    gtk_widget_show(availDevtitle);

    std::cout << "Bluetooth Devices Found: " << self->btInfo.availDev.size()
              << std::endl;
    for (auto [_, device] : self->btInfo.availDev) {
      std::string devLabel =
          device.name.length() > 0 ? device.name : device.addr;

      GtkWidget *deviceWid = gtk_button_new_with_label(devLabel.c_str());
      gtk_box_pack_start(GTK_BOX(self->btDevList), deviceWid, FALSE, FALSE, 2);
      gtk_widget_show(deviceWid);
    }
  }

  gtk_widget_show_all(self->btDevList);
}

void MainWindow::hideBTMenu(GtkWidget *widget, gpointer user_data) {
  MainWindow *self = static_cast<MainWindow *>(user_data);

  gtk_popover_popdown(GTK_POPOVER(self->btPopOverMenu));
}

void MainWindow::setupBT(GtkWidget *box, MainWindow *self) {
  GtkWidget *bt_img = gtk_button_new_with_label("");

  self->btPopOverMenu = gtk_popover_new(bt_img);
  gtk_popover_set_modal(GTK_POPOVER(self->btPopOverMenu), FALSE);
  gtk_popover_set_position(GTK_POPOVER(self->btPopOverMenu), GTK_POS_TOP);

  self->btDevList = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(self->btPopOverMenu), self->btDevList);
  gtk_widget_set_size_request(self->btPopOverMenu, 400, -1);

  g_signal_connect(bt_img, "enter", G_CALLBACK(showBTMenu), self);
  // g_signal_connect(self->btPopOverMenu, "closed", G_CALLBACK(hideBTMenu),
  // self);

  gtk_box_pack_start(GTK_BOX(box), bt_img, FALSE, FALSE, 0);
}
