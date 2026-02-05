#include "module.hpp"
#include "glib-object.h"
#include "gtk-layer-shell.h"
#include "gtk/gtk.h"
#include "manager.hpp"
#include <cstddef>
#include <string>

#define TAG "BluetoothModule"

BluetoothModule::BluetoothModule(AppContext *ctx, BluetoothManager *manager)
    : ctx(ctx) {
  btManager = manager;
}

void BluetoothModule::setupBT(GtkWidget *box) {
  GtkWidget *scanEBox = gtk_event_box_new();
  GtkWidget *img = gtk_label_new("");
  gtk_container_add(GTK_CONTAINER(scanEBox), img);
  gtk_widget_set_margin_start(scanEBox, 10);

  menuWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_layer_init_for_window(GTK_WINDOW(menuWin));

  gtk_layer_set_layer(GTK_WINDOW(menuWin), GTK_LAYER_SHELL_LAYER_TOP);
  gtk_layer_set_anchor(GTK_WINDOW(menuWin), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(menuWin), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
  // gtk_widget_set_margin_bottom(MenuWin, 30);

  // Main Box inside Window
  GtkWidget *mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(menuWin), mainBox);

  // Navigation Box with Close Button
  GtkWidget *navBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(mainBox), navBox, FALSE, FALSE, 0);
  gtk_widget_set_margin_bottom(navBox, 10);

  // Items in Nav Bar
  GtkWidget *title = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(title), "<big><b>Bluetooth Manager</b></big>");
  gtk_label_set_xalign(GTK_LABEL(title), 0);
  gtk_box_pack_start(GTK_BOX(navBox), title, FALSE, TRUE, 0);

  scanBtn = gtk_button_new_with_label(btManager->discovering ? "Stop" : "Scan");
  gtk_box_pack_end(GTK_BOX(navBox), scanBtn, FALSE, FALSE, 0);
  g_signal_connect(scanBtn, "clicked", G_CALLBACK(handleDiscovery), this);

  // Top Box with Power and Scan Buttons
  GtkWidget *topBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(mainBox), topBox, FALSE, FALSE, 0);

  powerBtn = gtk_check_button_new_with_label("Power");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(powerBtn), btManager->power);
  gtk_box_pack_start(GTK_BOX(topBox), powerBtn, FALSE, FALSE, 0);
  g_signal_connect(powerBtn, "clicked", G_CALLBACK(handlePower), this);

  devBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start(GTK_BOX(mainBox), devBox, FALSE, FALSE, 0);

  // Device List Sections
  pairedDevTitle = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(pairedDevTitle),
                       "<b><u>Paired Devices:</u></b>");
  gtk_widget_set_halign(pairedDevTitle, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(devBox), pairedDevTitle, FALSE, FALSE, 0);

  pairedDevList = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start(GTK_BOX(devBox), pairedDevList, FALSE, FALSE, 0);

  availDevTitle = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(availDevTitle),
                       "<b><u>Available Devices:</u></b>");
  gtk_widget_set_halign(availDevTitle, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(devBox), availDevTitle, FALSE, FALSE, 0);

  availDevList = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start(GTK_BOX(devBox), availDevList, FALSE, FALSE, 0);

  gtk_widget_show_all(mainBox);

  gtk_widget_hide(pairedDevTitle);
  gtk_widget_hide(pairedDevList);
  gtk_widget_hide(availDevTitle);
  gtk_widget_hide(availDevList);

  gtk_widget_set_size_request(menuWin, 400, 200);
  gtk_widget_set_margin_top(devBox, 20);
  gtk_widget_set_margin_top(mainBox, 5);
  gtk_widget_set_margin_bottom(mainBox, 5);
  gtk_widget_set_margin_start(mainBox, 10);
  gtk_widget_set_margin_end(mainBox, 10);

  g_signal_connect(scanEBox, "button-press-event",
                   G_CALLBACK(BluetoothModule::switchVisibilityBTMenu), this);

  gtk_grid_attach(GTK_GRID(box), scanEBox, 8, 0, 1, 1);
}

void BluetoothModule::switchVisibilityBTMenu(GtkWidget *widget, GdkEvent *e,
                                             gpointer user_data) {
  BluetoothModule *self = static_cast<BluetoothModule *>(user_data);

  if (!gtk_widget_get_visible(self->menuWin)) {
    self->updateBTList();
    gtk_widget_show(self->menuWin);
  } else
    gtk_widget_hide(self->menuWin);
}

void BluetoothModule::updateBTList() {
  if (!gtk_widget_get_visible(menuWin))
    return;

  GList *children = gtk_container_get_children(GTK_CONTAINER(availDevList));
  for (GList *iter = children; iter != nullptr; iter = iter->next) {
    gtk_widget_destroy(GTK_WIDGET(iter->data));
  }
  g_list_free(children);

  children = gtk_container_get_children(GTK_CONTAINER(pairedDevList));
  for (GList *iter = children; iter != nullptr; iter = iter->next) {
    gtk_widget_destroy(GTK_WIDGET(iter->data));
  }
  g_list_free(children);

  // Repopulate Device List
  int pairedDevs = 0, availDevs = 0;
  if (btManager->devices.size() > 0) {
    for (auto [_, device] : btManager->devices) {

      if (device.paired) {
        addDeviceEntry(device, pairedDevList, true);
        pairedDevs++;
      } else {
        addDeviceEntry(device, availDevList, false);
        availDevs++;
      }

      if (availDevs >= 15) {
        break; // Limit to 15 new devices shown
      }
    }
  }

  if (pairedDevs > 0) {
    gtk_widget_show(pairedDevTitle);
    gtk_widget_show(pairedDevList);
  } else {
    gtk_widget_hide(pairedDevTitle);
    gtk_widget_hide(pairedDevList);
  }

  if (availDevs > 0) {
    gtk_widget_show(availDevTitle);
    gtk_widget_show(availDevList);
  } else {
    gtk_widget_hide(availDevTitle);
    gtk_widget_hide(availDevList);
  }
}

void BluetoothModule::addDeviceEntry(const Device &dev, GtkWidget *parentBox,
                                     bool isPaired) {

  GtkWidget *devSection = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  // Device Label
  std::string devLabel = dev.name.length() > 0 ? dev.name : dev.addr;
  if (devLabel.size() > 20) {
    devLabel = devLabel.substr(0, 17) + "...";
  }

  // Tooltip Info
  std::string devTooltip = "Address: " + dev.addr;
  if (dev.batteryPer != -1)
    devTooltip += "\nBattery: " + std::to_string(dev.batteryPer) + "%";
  if (dev.rssi != -110)
    devTooltip += "\nSignal Strength: " + std::to_string(dev.rssi) + " dBm";

  // Adding Label and Tooltip
  GtkWidget *devLabelWid = gtk_label_new(devLabel.c_str());
  gtk_box_pack_start(GTK_BOX(devSection), devLabelWid, FALSE, FALSE, 2);
  gtk_widget_set_tooltip_text(devLabelWid, devTooltip.c_str());

  // Only Shown for Paired Devices
  if (dev.paired) {
    // Device Unpair Button
    GtkWidget *devUnpairBtn = gtk_button_new_with_label("✖");
    gtk_box_pack_end(GTK_BOX(devSection), devUnpairBtn, FALSE, FALSE, 2);

    // Device Trust Button
    GtkWidget *devTrustBtn = gtk_button_new_with_label("");
    gtk_box_pack_end(GTK_BOX(devSection), devTrustBtn, FALSE, FALSE, 2);
  }

  // Device Connect Button Connect Icon
  GtkWidget *devConnBtn = gtk_button_new_with_label("");
  gtk_box_pack_end(GTK_BOX(devSection), devConnBtn, FALSE, FALSE, 2);

  // Connect Signal
  FuncArgs *args = g_new0(FuncArgs, 1);
  args->devIfacePath = g_strdup(dev.path.c_str());
  args->state = !dev.connected;
  args->btManager = btManager;
  args->ctx = ctx;
  g_signal_connect_data(devConnBtn, "clicked",
                        G_CALLBACK(BluetoothModule::handleDeviceConnect), args,
                        (GClosureNotify)FreeArgs, (GConnectFlags)0);

  gtk_box_pack_start(GTK_BOX(parentBox), devSection, FALSE, FALSE, 2);
  gtk_widget_show_all(devSection);
}

void BluetoothModule::handleDiscovery(GtkWidget *widget, gpointer user_data) {
  BluetoothModule *self = static_cast<BluetoothModule *>(user_data);

  if (self->btManager->discovering) {
    self->ctx->logger.LogInfo(TAG, "Stopping Bluetooth Discovery.");

    if (self->btManager->switchDiscovery(false) == 0)
      self->ctx->logger.LogError(TAG, "Bluetooth Discovery Stopped.");
  } else {
    self->ctx->logger.LogInfo(TAG, "Starting Bluetooth Discovery.");

    if (self->btManager->switchDiscovery(true) == 0) {
      self->ctx->logger.LogInfo(TAG, "Bluetooth Discovery Started.");
      self->updateBTList();
    }
  }

  gtk_button_set_label(GTK_BUTTON(self->scanBtn),
                       self->btManager->discovering ? "Stop" : "Scan");
}

void BluetoothModule::handlePower(GtkWidget *widget, gpointer user_data) {
  BluetoothModule *self = static_cast<BluetoothModule *>(user_data);

  gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  if (!self->btManager->switchPower(active)) {
    std::string msg = "Bluetooth Power Switched ";
    msg += (active ? "ON" : "OFF");
    self->ctx->logger.LogInfo(TAG, msg);
    self->ctx->showUpdateWindow(UpdateModule::BLUETOOTH, active ? "base" : "disabled", msg);
  }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
                               self->btManager->power);
}

void BluetoothModule::handleDeviceConnect(GtkWidget *widget,
                                          gpointer user_data) {
  FuncArgs *args = static_cast<FuncArgs *>(user_data);

  args->btManager->connectDevice(args->state, args->devIfacePath);
}

void BluetoothModule::FreeArgs(gpointer data) {
  FuncArgs *args = static_cast<FuncArgs *>(data);
  g_free(args->devIfacePath);
  g_free(args);
}
