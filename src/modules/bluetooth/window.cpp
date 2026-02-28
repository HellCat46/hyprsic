#include "window.hpp"
#include "manager.hpp"

#define TAG "BluetoothWindow"

BluetoothWindow::BluetoothWindow(AppContext *ctx, BluetoothManager* manager) : ctx(ctx), manager(manager){}

void BluetoothWindow::init() {
    menuBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  
    // Navigation Box with Close Button
    GtkWidget *navBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(menuBox), navBox, FALSE, FALSE, 0);
    gtk_widget_set_margin_bottom(navBox, 10);
  
    // Items in Nav Bar
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<big><b>Bluetooth Manager</b></big>");
    gtk_label_set_xalign(GTK_LABEL(title), 0);
    gtk_box_pack_start(GTK_BOX(navBox), title, FALSE, TRUE, 0);
  
    scanBtn = gtk_button_new_with_label(manager->discovering ? "Stop" : "Scan");
    gtk_box_pack_end(GTK_BOX(navBox), scanBtn, FALSE, FALSE, 0);
    g_signal_connect(scanBtn, "clicked", G_CALLBACK(handleDiscovery), this);
  
    // Top Box with Power and Scan Buttons
    GtkWidget *topBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(menuBox), topBox, FALSE, FALSE, 0);
  
    // Power Toggle
    GtkWidget *powerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *powerLbl = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(powerLbl), "<b>Power</b>");
    gtk_widget_set_halign(powerLbl, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(powerBox), powerLbl, FALSE, FALSE, 0);
  
    powerBtn = gtk_switch_new();
    gtk_switch_set_state(GTK_SWITCH(powerBtn), manager->power);
    gtk_box_pack_end(GTK_BOX(topBox), powerBtn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(topBox), powerBox, FALSE, FALSE, 0);
    g_signal_connect(powerBtn, "state-set", G_CALLBACK(handlePower), this);
  
    devBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(menuBox), devBox, FALSE, FALSE, 0);
  
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
  
    GtkWidget *availDevScroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(availDevScroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(availDevScroll, 200, 150);
    availDevList = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(availDevScroll), availDevList);
    gtk_box_pack_start(GTK_BOX(devBox), availDevScroll, FALSE, FALSE, 0);
  
    gtk_widget_show_all(menuBox);
  
    gtk_widget_hide(pairedDevTitle);
    gtk_widget_hide(pairedDevList);
    gtk_widget_hide(availDevTitle);
    gtk_widget_hide(availDevList);
  
    gtk_widget_set_margin_top(devBox, 20);
    gtk_widget_set_margin_top(menuBox, 5);
    gtk_widget_set_margin_bottom(menuBox, 5);
    gtk_widget_set_margin_start(menuBox, 10);
    gtk_widget_set_margin_end(menuBox, 10);
  
    
    ctx->addModule(menuBox, "bluetooth");
    update(true);

}


void BluetoothWindow::update(bool force) {
  if (!gtk_widget_get_visible(menuBox) && !force)
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
  if (manager->devices.size() > 0) {
    for (auto [_, device] : manager->devices) {

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


void BluetoothWindow::addDeviceEntry(const Device &dev, GtkWidget *parentBox,
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
    GtkWidget *devRemoveBtn = gtk_button_new_with_label("✖");
    gtk_box_pack_end(GTK_BOX(devSection), devRemoveBtn, FALSE, FALSE, 2);
    gtk_widget_set_tooltip_text(devRemoveBtn, "Remove Device");

    // Connect Signal
    FuncArgs *rmvArgs = g_new0(FuncArgs, 1);
    rmvArgs->devIfacePath = g_strdup(dev.path.c_str());
    rmvArgs->manager = manager;
    rmvArgs->ctx = ctx;
    g_signal_connect_data(devRemoveBtn, "clicked",
                          G_CALLBACK(BluetoothWindow::handleDeviceRemove),
                          rmvArgs, (GClosureNotify)FreeArgs, (GConnectFlags)0);

    // Device Trust Button
    GtkWidget *devTrustBtn = gtk_button_new_with_label("");
    gtk_box_pack_end(GTK_BOX(devSection), devTrustBtn, FALSE, FALSE, 2);
    gtk_widget_set_tooltip_text(devTrustBtn, dev.trusted ? "Untrust Device"
                                                         : "Trust Device");

    // Connect Signal
    FuncArgs *trustArgs = g_new0(FuncArgs, 1);
    trustArgs->devIfacePath = g_strdup(dev.path.c_str());
    trustArgs->state = !dev.trusted;
    trustArgs->manager = manager;
    trustArgs->ctx = ctx;
    g_signal_connect_data(
        devTrustBtn, "clicked", G_CALLBACK(BluetoothWindow::handleDeviceTrust),
        trustArgs, (GClosureNotify)FreeArgs, (GConnectFlags)0);
  }

  // Device Connect Button Connect Icon
  GtkWidget *devConnBtn = gtk_button_new_with_label("");
  gtk_box_pack_end(GTK_BOX(devSection), devConnBtn, FALSE, FALSE, 2);
  gtk_widget_set_tooltip_text(devConnBtn, dev.connected ? "Disconnect Device"
                                                        : "Connect Device");

  // Connect Signal
  FuncArgs *args = g_new0(FuncArgs, 1);
  args->devIfacePath = g_strdup(dev.path.c_str());
  args->state = !dev.connected;
  args->manager = manager;
  args->ctx = ctx;
  g_signal_connect_data(devConnBtn, "clicked",
                        G_CALLBACK(BluetoothWindow::handleDeviceConnect), args,
                        (GClosureNotify)FreeArgs, (GConnectFlags)0);

  gtk_box_pack_start(GTK_BOX(parentBox), devSection, FALSE, FALSE, 2);
  gtk_widget_show_all(devSection);
}

void BluetoothWindow::handleDiscovery(GtkWidget *widget, gpointer user_data) {
  BluetoothWindow *self = static_cast<BluetoothWindow *>(user_data);

  if (self->manager->discovering) {
    self->ctx->logger.LogInfo(TAG, "Stopping Bluetooth Discovery.");

    if (self->manager->switchDiscovery(false) == 0)
      self->ctx->logger.LogError(TAG, "Bluetooth Discovery Stopped.");
  } else {
    self->ctx->logger.LogInfo(TAG, "Starting Bluetooth Discovery.");

    if (self->manager->switchDiscovery(true) == 0) {
      self->ctx->logger.LogInfo(TAG, "Bluetooth Discovery Started.");
      self->update();
    }
  }

  gtk_button_set_label(GTK_BUTTON(self->scanBtn),
                       self->manager->discovering ? "Stop" : "Scan");
}

void BluetoothWindow::handlePower(GtkSwitch *widget, gboolean state,
                                  gpointer user_data) {
  BluetoothWindow *self = static_cast<BluetoothWindow *>(user_data);

  if (!self->manager->switchPower(state)) {
    std::string msg = "Bluetooth Power Switched ";
    msg += (state ? "ON" : "OFF");
    self->ctx->logger.LogInfo(TAG, msg);
    self->ctx->showUpdateWindow(UpdateModule::BLUETOOTH,
                                state ? "base" : "disabled", msg);
  }
}

void BluetoothWindow::handleDeviceTrust(GtkWidget *widget, gpointer user_data) {
  FuncArgs *args = static_cast<FuncArgs *>(user_data);

  args->manager->trustDevice(args->state, args->devIfacePath);
}

void BluetoothWindow::handleDeviceRemove(GtkWidget *widget,
                                         gpointer user_data) {
  FuncArgs *args = static_cast<FuncArgs *>(user_data);

  args->manager->removeDevice(args->devIfacePath);
}

void BluetoothWindow::handleDeviceConnect(GtkWidget *widget,
                                          gpointer user_data) {
  FuncArgs *args = static_cast<FuncArgs *>(user_data);

  args->manager->connectDevice(args->state, args->devIfacePath);
}

void BluetoothWindow::FreeArgs(gpointer data) {
  FuncArgs *args = static_cast<FuncArgs *>(data);
  g_free(args->devIfacePath);
  g_free(args);
}
