#include "module.hpp"
#include "gtk/gtk.h"
#include <cstddef>

#define TAG "BluetoothModule"

BluetoothModule::BluetoothModule(AppContext *ctx) : btManager(ctx, &ctx->logging), logger(&ctx->logging) {}

void BluetoothModule::setupBT(GtkWidget *box) {
  GtkWidget *bt_img = gtk_button_new_with_label("");

  btPopOverMenu = gtk_popover_new(bt_img);
  gtk_popover_set_modal(GTK_POPOVER(btPopOverMenu), FALSE);
  gtk_popover_set_position(GTK_POPOVER(btPopOverMenu), GTK_POS_TOP);

  
  // Main Box inside Popover
  GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(btPopOverMenu), main_box);

  
  // Navigation Box with Close Button
  GtkWidget *nav_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(main_box), nav_box, FALSE, FALSE, 0);
  gtk_widget_set_margin_bottom(nav_box, 10);
  
  
  // Items in Nav Bar
  GtkWidget *title = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(title), "<b>Bluetooth Manager</b>");
  gtk_label_set_xalign(GTK_LABEL(title), 0);
  gtk_box_pack_start(GTK_BOX(nav_box), title, FALSE, TRUE, 0);
  
  GtkWidget *closeBtn = gtk_button_new_with_label("✖");
  gtk_box_pack_end(GTK_BOX(nav_box), closeBtn, FALSE, FALSE, 0);
  btScanBtn =
      gtk_button_new_with_label(btManager.discovering ? "Stop" : "Scan");
  gtk_box_pack_end(GTK_BOX(nav_box), btScanBtn, FALSE, FALSE, 0);
  g_signal_connect(btScanBtn, "clicked", G_CALLBACK(handleDiscovery), this);
  
  // Top Box with Power and Scan Buttons
  GtkWidget *top_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(main_box), top_box, FALSE, FALSE, 0);

  btPowerBtn = gtk_check_button_new_with_label("Power");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btPowerBtn), btManager.power);
  gtk_box_pack_start(GTK_BOX(top_box), btPowerBtn, FALSE, FALSE, 0);
  g_signal_connect(btPowerBtn, "clicked", G_CALLBACK(handlePower), this);
  
  


  
  btDevList = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start(GTK_BOX(main_box), btDevList, FALSE, FALSE, 0);

  gtk_widget_show_all(main_box);

  gtk_widget_set_size_request(btPopOverMenu, 400, 200);
  gtk_widget_set_margin_top(btDevList, 20);
  gtk_widget_set_margin_top(main_box, 5);
  gtk_widget_set_margin_bottom(main_box, 5);
  gtk_widget_set_margin_start(main_box, 10);
  gtk_widget_set_margin_end(main_box, 10);

  g_signal_connect(bt_img, "enter", G_CALLBACK(showBTMenu), this);
  g_signal_connect(closeBtn, "clicked", G_CALLBACK(hideBTMenu), this);

  gtk_box_pack_start(GTK_BOX(box), bt_img, FALSE, FALSE, 0);
}

void BluetoothModule::showBTMenu(GtkWidget *widget, gpointer user_data) {
  BluetoothModule *self = static_cast<BluetoothModule *>(user_data);

  self->updateBTList();

  gtk_popover_popup(GTK_POPOVER(self->btPopOverMenu));
}

void BluetoothModule::updateBTList() {

  GList *children = gtk_container_get_children(GTK_CONTAINER(btDevList));
  for (GList *iter = children; iter != nullptr; iter = iter->next) {
    gtk_widget_destroy(GTK_WIDGET(iter->data));
  }
  g_list_free(children);

  btManager.getDeviceList();
  // btManager.printDevicesInfo();

  // Known Devices Section
  if (btManager.devices.size() > 0) {
    GtkWidget *knownDevtitle = gtk_label_new("Known Devices: ");
    gtk_box_pack_start(GTK_BOX(btDevList), knownDevtitle, FALSE, FALSE, 0);
    gtk_widget_show(knownDevtitle);

    for (auto [_, device] : btManager.devices) {
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
      args->btManager = &btManager;
      g_signal_connect(devConnBtn, "clicked",
                       G_CALLBACK(BluetoothModule::handleDeviceConnect), args);

      gtk_box_pack_start(GTK_BOX(btDevList), devSection, FALSE, FALSE, 2);
      gtk_widget_show_all(devSection);
    }
  }

  // Available Devices Section
  if (btManager.devices.size() > 0) {
    GtkWidget *availDevtitle = gtk_label_new("Available Devices: ");
    gtk_box_pack_start(GTK_BOX(btDevList), availDevtitle, FALSE, FALSE, 0);
    gtk_widget_show(availDevtitle);

    for (auto [_, device] : btManager.devices) {
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
      args->btManager = &btManager;

      g_signal_connect(devConnBtn, "clicked",
                       G_CALLBACK(BluetoothModule::handleDeviceConnect), args);

      gtk_box_pack_start(GTK_BOX(btDevList), devSection, FALSE, FALSE, 2);
      gtk_widget_show_all(devSection);
    }
  }

  gtk_widget_show_all(btDevList);
}

void BluetoothModule::hideBTMenu(GtkWidget *widget, gpointer user_data) {
  BluetoothModule *self = static_cast<BluetoothModule *>(user_data);

  if (gtk_widget_get_visible(self->btPopOverMenu))
    gtk_popover_popdown(GTK_POPOVER(self->btPopOverMenu));
}

void BluetoothModule::handleDiscovery(GtkWidget *widget, gpointer user_data) {
  BluetoothModule *self = static_cast<BluetoothModule *>(user_data);

  if (self->btManager.discovering) {
    self->logger->LogInfo(TAG, "Stopping Bluetooth Discovery.");

    if (self->btManager.switchDiscovery(false) == 0)
      self->logger->LogError(TAG, "Bluetooth Discovery Stopped.");
  } else {
    self->logger->LogInfo(TAG, "Starting Bluetooth Discovery.");

    if (self->btManager.switchDiscovery(true) == 0) {
      self->logger->LogInfo(TAG, "Bluetooth Discovery Started.");
      self->updateBTList();
    }
  }

  gtk_button_set_label(GTK_BUTTON(self->btScanBtn),
                       self->btManager.discovering ? "Stop" : "Scan");
}

void BluetoothModule::handlePower(GtkWidget *widget, gpointer user_data) {
  BluetoothModule *self = static_cast<BluetoothModule *>(user_data);

  gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  if (!self->btManager.switchPower(active)) {
    std::string msg = "Bluetooth Power Switched ";
    msg += (active ? "ON" : "OFF");
    self->logger->LogInfo(TAG, msg);
  }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
                               self->btManager.power);
}

void BluetoothModule::handleDeviceConnect(GtkWidget *widget,
                                          gpointer user_data) {
  FuncArgs *args = static_cast<FuncArgs *>(user_data);

  args->btManager->connectDevice(args->state, args->devIfacePath);
}