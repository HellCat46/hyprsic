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

MainWindow::MainWindow() : btManager(&ctx) {
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
  self->window = gtk_application_window_new(app);

  gtk_layer_init_for_window(GTK_WINDOW(self->window));

  gtk_layer_set_layer(GTK_WINDOW(self->window), GTK_LAYER_SHELL_LAYER_TOP);
  gtk_layer_set_anchor(GTK_WINDOW(self->window), GTK_LAYER_SHELL_EDGE_BOTTOM,
                       TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(self->window), GTK_LAYER_SHELL_EDGE_LEFT,
                       TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(self->window), GTK_LAYER_SHELL_EDGE_RIGHT,
                       TRUE);
  gtk_layer_set_exclusive_zone(GTK_WINDOW(self->window), 30);

  GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add(GTK_CONTAINER(self->window), main_box);
  std::string txt = "";

  // Left Box to Show Workspaces Info
  self->workspaceSecWid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  gtk_box_pack_start(GTK_BOX(main_box), self->workspaceSecWid, FALSE, FALSE, 5);

  // Update Workspaces Info
  self->setupWorkspaces();

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

  gtk_widget_show_all(self->window);
}

gboolean MainWindow::UpdateData(gpointer data) {
  MainWindow *self = static_cast<MainWindow *>(data);
  std::string txt = "";
  
  self->setupWorkspaces();

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

  updateBTList(self);

  return true;
}

void MainWindow::showBTMenu(GtkWidget *widget, gpointer user_data) {
  MainWindow *self = static_cast<MainWindow *>(user_data);

  updateBTList(self);

  gtk_popover_popup(GTK_POPOVER(self->btPopOverMenu));
}

void MainWindow::updateBTList(MainWindow *self) {

  GList *children = gtk_container_get_children(GTK_CONTAINER(self->btDevList));
  for (GList *iter = children; iter != NULL; iter = iter->next) {
    gtk_widget_destroy(GTK_WIDGET(iter->data));
  }
  g_list_free(children);

  self->btManager.getDeviceList();
  // self->btManager.printDevicesInfo();

  // Known Devices Section
  if (self->btManager.devices.size() > 0) {
    GtkWidget *knownDevtitle = gtk_label_new("Known Devices: ");
    gtk_box_pack_start(GTK_BOX(self->btDevList), knownDevtitle, FALSE, FALSE,
                       0);
    gtk_widget_show(knownDevtitle);

    for (auto [_, device] : self->btManager.devices) {
      if (!device.paired)
        continue; // Improve it later on. Combine both Devices Loops
      GtkWidget *devSection = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

      // Device Label
      std::string devLabel =
          device.name.length() > 0 ? device.name : device.addr;
      if (devLabel.size() > 20) {
        devLabel = devLabel.substr(0, 17) + "...";
      }
      std::string devTooltip = "Address: " + device.addr;
      if (device.batteryPer != -1)
        devTooltip += "\nBattery: " + std::to_string(device.batteryPer) + "%";
      if (device.rssi != -110)
        devTooltip +=
            "\nSignal Strength: " + std::to_string(device.rssi) + " dBm";

      GtkWidget *devLabelWid = gtk_label_new(devLabel.c_str());
      gtk_box_pack_start(GTK_BOX(devSection), devLabelWid, FALSE, FALSE, 2);
      gtk_widget_set_tooltip_text(devLabelWid, devTooltip.c_str());

      // Device Unpair Button
      GtkWidget *devUnpairBtn = gtk_button_new_with_label("✖");
      gtk_box_pack_end(GTK_BOX(devSection), devUnpairBtn, FALSE, FALSE, 2);

      // Device Trust Button
      GtkWidget *devTrustBtn = gtk_button_new_with_label("");
      gtk_box_pack_end(GTK_BOX(devSection), devTrustBtn, FALSE, FALSE, 2);

      // Device Connect Button Connect Icon
      GtkWidget *devConnBtn = gtk_button_new_with_label("");
      gtk_box_pack_end(GTK_BOX(devSection), devConnBtn, FALSE, FALSE, 2);

      FuncArgs *args = g_new0(FuncArgs, 1);
      args->devIfacePath = g_strdup(device.path.c_str());
      args->state = !device.connected;
      args->ctx = &self->ctx;
      g_signal_connect(devConnBtn, "clicked",
                       G_CALLBACK(BluetoothManager::connectDevice), args);

      gtk_box_pack_start(GTK_BOX(self->btDevList), devSection, FALSE, FALSE, 2);
      gtk_widget_show_all(devSection);
    }
  }

  // Available Devices Section
  if (self->btManager.devices.size() > 0) {
    GtkWidget *availDevtitle = gtk_label_new("Available Devices: ");
    gtk_box_pack_start(GTK_BOX(self->btDevList), availDevtitle, FALSE, FALSE,
                       0);
    gtk_widget_show(availDevtitle);

    // std::cout << "Bluetooth Devices Found: " <<
    // self->btManager.devices.size()
    //           << std::endl;
    for (auto [_, device] : self->btManager.devices) {
      if (device.paired)
        continue; // Improve it later on. Combine both Devices Loops

      GtkWidget *devSection = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
      std::string devLabel =
          device.name.length() > 0 ? device.name : device.addr;
      GtkWidget *deviceWid = gtk_label_new(devLabel.c_str());
      gtk_box_pack_start(GTK_BOX(devSection), deviceWid, FALSE, FALSE, 2);
      gtk_widget_show(deviceWid);

      // Device Connect Button Connect Icon
      GtkWidget *devConnBtn = gtk_button_new_with_label("");
      gtk_box_pack_end(GTK_BOX(devSection), devConnBtn, FALSE, FALSE, 2);

      FuncArgs *args = g_new0(FuncArgs, 1);
      args->devIfacePath = g_strdup(device.path.c_str());
      args->state = !device.connected;
      args->ctx = &self->ctx;
      g_signal_connect(devConnBtn, "clicked",
                       G_CALLBACK(BluetoothManager::connectDevice), args);

      gtk_box_pack_start(GTK_BOX(self->btDevList), devSection, FALSE, FALSE, 2);
      gtk_widget_show_all(devSection);
    }
  }

  gtk_widget_show_all(self->btDevList);
}

void MainWindow::hideBTMenu(GtkWidget *widget, gpointer user_data) {
  MainWindow *self = static_cast<MainWindow *>(user_data);

  if (gtk_widget_get_visible(self->btPopOverMenu))
    gtk_popover_popdown(GTK_POPOVER(self->btPopOverMenu));
}

void MainWindow::setupBT(GtkWidget *box, MainWindow *self) {
  GtkWidget *bt_img = gtk_button_new_with_label("");

  self->btPopOverMenu = gtk_popover_new(bt_img);
  gtk_popover_set_modal(GTK_POPOVER(self->btPopOverMenu), FALSE);
  gtk_popover_set_position(GTK_POPOVER(self->btPopOverMenu), GTK_POS_TOP);

  // Main Box inside Popover
  GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(self->btPopOverMenu), main_box);

  
  // Navigation Box with Close Button
  GtkWidget *nav_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(main_box), nav_box, FALSE, FALSE, 0);
  gtk_widget_set_margin_bottom(nav_box, 10);
  GtkWidget *closeBtn = gtk_button_new_with_label("✖");
  gtk_box_pack_end(GTK_BOX(nav_box), closeBtn, FALSE, FALSE, 0);

  
  // Top Box with Power and Scan Buttons
  GtkWidget *top_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(main_box), top_box, FALSE, FALSE, 0);

  self->btPowerBtn = gtk_check_button_new_with_label("Power");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->btPowerBtn),
                               self->btManager.power);
  gtk_box_pack_start(GTK_BOX(top_box), self->btPowerBtn, FALSE, FALSE, 0);
  g_signal_connect(self->btPowerBtn, "clicked", G_CALLBACK(handlePower), self);

  self->btScanBtn =
      gtk_button_new_with_label(self->btManager.discovering ? "Stop" : "Scan");
  gtk_box_pack_end(GTK_BOX(top_box), self->btScanBtn, FALSE, FALSE, 0);
  g_signal_connect(self->btScanBtn, "clicked", G_CALLBACK(handleDiscovery),
                   self);

  self->btDevList = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start(GTK_BOX(main_box), self->btDevList, FALSE, FALSE, 0);

  gtk_widget_show_all(main_box);

  gtk_widget_set_size_request(self->btPopOverMenu, 400, 200);
  gtk_widget_set_margin_top(self->btDevList, 20);
  gtk_widget_set_margin_top(main_box, 5);
  gtk_widget_set_margin_bottom(main_box, 5);
  gtk_widget_set_margin_start(main_box, 10);
  gtk_widget_set_margin_end(main_box, 10);

  g_signal_connect(bt_img, "enter", G_CALLBACK(showBTMenu), self);
  g_signal_connect(closeBtn, "clicked", G_CALLBACK(hideBTMenu), self);

  gtk_box_pack_start(GTK_BOX(box), bt_img, FALSE, FALSE, 0);
}


void MainWindow::handleDiscovery(GtkWidget *widget, gpointer user_data) {
  MainWindow *self = static_cast<MainWindow *>(user_data);

  if (self->btManager.discovering) {
    std::cout << "[Info] Stopping Bluetooth Discovery." << std::endl;

    if (self->btManager.switchDiscovery(false) == 0)
      std::cerr << "[Error] Bluetooth Discovery Stopped." << std::endl;
  } else {
    std::cout << "[Info] Starting Bluetooth Discovery." << std::endl;

    if (self->btManager.switchDiscovery(true) == 0) {
      std::cout << "[Info] Bluetooth Discovery Started." << std::endl;
      updateBTList(self);
    }
  }

  gtk_button_set_label(GTK_BUTTON(self->btScanBtn),
                       self->btManager.discovering ? "Stop" : "Scan");
}


void MainWindow::handlePower(GtkWidget *widget, gpointer user_data) {
  MainWindow *self = static_cast<MainWindow *>(user_data);

  gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  if (!self->btManager.switchPower(active)) {
    std::cout << "[Info] Bluetooth Power Switched " << (active ? "ON" : "OFF")
              << std::endl;
  }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
                               self->btManager.power);
}


void MainWindow::setupWorkspaces() {
    std::string txt = ""; 
    if (!this->hyprWS.GetWorkspaces()) {
      GList *child =
          gtk_container_get_children(GTK_CONTAINER(this->workspaceSecWid));
      for (GList *iter = child; iter != NULL; iter = iter->next) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
      }
      g_list_free(child);
  
      for (auto workspace : this->hyprWS.workspaces) {
        txt = std::to_string(workspace.first) + " ";
  
        if (this->hyprWS.activeWorkspaceId == workspace.first) {
          txt = "[ " + txt + "]";
        }
  
        GtkWidget *evtBox = gtk_event_box_new();
        GtkWidget *wsLabel = gtk_label_new(txt.c_str());
        
        gtk_container_add(GTK_CONTAINER(evtBox), wsLabel);
        gtk_box_pack_start(GTK_BOX(this->workspaceSecWid), evtBox, FALSE, FALSE,
                           0);
    
        
        ChgWSArgs* args = g_new0(ChgWSArgs, 1);
        args->wsInstance = &this->hyprWS;
        args->wsId = workspace.first;
       
        g_signal_connect_data(evtBox, "button-press-event", G_CALLBACK(MainWindow::chgWorkspace), args, (GClosureNotify)g_free, (GConnectFlags)0);
      }
      gtk_widget_show_all(this->workspaceSecWid);
    }
}

void MainWindow::chgWorkspace(GtkWidget *widget, GdkEvent* e, gpointer user_data){
    ChgWSArgs* args = static_cast<ChgWSArgs*>(user_data);
    
    if(args->wsInstance->SwitchToWorkspace(args->wsInstance, args->wsId) != 0){
        std::cerr<<"[Error] Failed to Switch Workspace"<<std::endl;
    }
}