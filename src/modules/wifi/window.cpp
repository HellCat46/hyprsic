#include "app/window.hpp"
#include "glib-object.h"
#include "gtk/gtk.h"
#include "modules/wifi/window.hpp"
#include <algorithm>
#include <cstring>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#define TAG "WifiWindow"

WifiWindow::WifiWindow(AppContext *ctx, WifiManager *mgr)
    : ctx(ctx), manager(mgr) {}

void WifiWindow::init() {
  mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(mainBox, 10);
  gtk_widget_set_margin_end(mainBox, 10);
  gtk_widget_set_margin_top(mainBox, 10);
  gtk_widget_set_margin_bottom(mainBox, 10);

  // Top Bar with Title and Scan Button
  GtkWidget *topbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(mainBox), topbar, false, false, 0);

  GtkWidget *title = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(title), "<b><i>Wi-Fi Networks</i></b>");
  gtk_box_pack_start(GTK_BOX(topbar), title, false, false, 0);

  scanBtn = gtk_button_new_with_label("Scan");
  gtk_box_pack_end(GTK_BOX(topbar), scanBtn, false, false, 0);
  g_signal_connect(scanBtn, "clicked", G_CALLBACK(handleScan), this);

  // Power Control
  GtkWidget *powerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(mainBox), powerBox, false, false, 0);

  GtkWidget *powerLbl = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(powerLbl), "<b>Power</b>");
  gtk_widget_set_halign(powerLbl, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(powerBox), powerLbl, false, false, 0);

  GtkWidget *powerBtn = gtk_switch_new();
  gtk_switch_set_state(GTK_SWITCH(powerBtn), manager->IsPowered());
  gtk_box_pack_end(GTK_BOX(powerBox), powerBtn, false, false, 0);

  // Connected Device Box
  connDevBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start(GTK_BOX(mainBox), connDevBox, false, false, 0);
  gtk_widget_set_margin_top(connDevBox, 20);

  GtkWidget *connDevTitle = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(connDevTitle), "<u>Connected Network:</u>");
  gtk_widget_set_halign(connDevTitle, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(connDevBox), connDevTitle, false, false, 0);

  connDevIBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(connDevBox), connDevIBox, false, false, 0);

  connDeviceName = gtk_label_new(nullptr);
  gtk_widget_set_halign(connDeviceName, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(connDevIBox), connDeviceName, true, true, 0);

  frgtBtn = gtk_button_new_with_label("Forget");
  gtk_box_pack_end(GTK_BOX(connDevIBox), frgtBtn, false, false, 5);
  connDevFrgtId = g_signal_connect_data(
      frgtBtn, "clicked", G_CALLBACK(handleForget),
      new ActionArgs{.manager = manager, .devPath = manager->connDev},
      (GClosureNotify)FreeActionArgs, (GConnectFlags)0);

  GtkWidget *disCBtn = gtk_button_new_with_label("Disconnect");
  gtk_box_pack_end(GTK_BOX(connDevIBox), disCBtn, false, false, 5);
  g_signal_connect(disCBtn, "clicked", G_CALLBACK(handleDisconnect), this);

  // Available Networks
  devBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start(GTK_BOX(mainBox), devBox, false, false, 0);
  gtk_widget_set_margin_top(devBox, 20);

  GtkWidget *availDevTitle = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(availDevTitle), "<u>Available Networks:</u>");
  gtk_widget_set_halign(availDevTitle, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(devBox), availDevTitle, false, false, 0);

  GtkWidget *devListScrlBox = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(devListScrlBox),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(devListScrlBox, 400, 200);
  gtk_box_pack_start(GTK_BOX(devBox), devListScrlBox, false, false, 0);

  devListBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(devListScrlBox), devListBox);

  // Passphrase Input Box
  passEntBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(devBox), passEntBox, false, false, 5);

  passEntry = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(passEntry), false);
  gtk_entry_set_placeholder_text(GTK_ENTRY(passEntry), "Enter Password");
  gtk_box_pack_start(GTK_BOX(passEntBox), passEntry, true, true, 5);

  GtkWidget *passEntBtn = gtk_button_new_with_label("Submit");
  gtk_box_pack_end(GTK_BOX(passEntBox), passEntBtn, false, false, 5);
  g_signal_connect(passEntBtn, "clicked", G_CALLBACK(handlePassSubmit), this);

  gtk_widget_show_all(mainBox);
  gtk_widget_hide(connDevBox);
  gtk_widget_hide(devBox);
  gtk_widget_hide(passEntBox);

  ctx->addModule(mainBox, "wifi");

  update();
}

void WifiWindow::update() {
  updateConnDev();

  if (!manager->IsScanning()) {
    gtk_widget_set_sensitive(scanBtn, true);
  }

  if (!manager->authDev.empty()) {
    gtk_widget_show(passEntBox);
  } else {
    gtk_widget_hide(passEntBox);
  }

  GList *children = gtk_container_get_children(GTK_CONTAINER(devListBox));
  for (GList *child = children; child; child = child->next) {
    gtk_widget_destroy(GTK_WIDGET(child->data));
  }
  g_list_free(children);

  std::vector<std::pair<short, GtkWidget *>> devWids;
  for (const auto &[devPath, station] : manager->devices) {
    if (devPath == manager->connDev)
      continue;

    devWids.push_back({station.rssi, addDevList(devPath, station)});
  }

  // Sort devices by signal strength
  std::sort(devWids.begin(), devWids.end(),
            std::greater<std::pair<short, GtkWidget *>>());
  for (const auto &[_, widget] : devWids) {
    gtk_box_pack_start(GTK_BOX(devListBox), widget, false, false, 5);
  }

  if (devWids.size() > 0) {
    gtk_widget_show_all(devListBox);
  } else {
    gtk_widget_hide(devListBox);
  }
}

void WifiWindow::updateConnDev() {

  auto it = manager->devices.find(manager->connDev);
  if (it != manager->devices.end()) {

    WifiStation station = it->second;
    gtk_label_set_markup(GTK_LABEL(connDeviceName),
                         ("<b>" + station.ssid + "</b>").c_str());
    addTooltip(connDeviceName, station);

    // Disconnect the last "Forget" signal handler to stop memory leak while
    // also updating it
    g_signal_handler_disconnect(frgtBtn, connDevFrgtId);
    connDevFrgtId = g_signal_connect_data(
        frgtBtn, "clicked", G_CALLBACK(handleForget),
        new ActionArgs{.manager = manager, .devPath = manager->connDev},
        (GClosureNotify)FreeActionArgs, (GConnectFlags)0);

    gtk_widget_show_all(connDevBox);
  } else {
    gtk_widget_hide(connDevBox);
  }
}

GtkWidget *WifiWindow::addDevList(const std::string &devPath,
                                  const WifiStation &station) {
  GtkWidget *devRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  GtkWidget *devName = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(devName),
                       ("<b>" + station.ssid + "</b>").c_str());
  addTooltip(devName, station);
  
  gtk_widget_set_halign(devName, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(devRow), devName, true, true, 5);

  GtkWidget *connBtn = gtk_button_new_with_label("Connect");
  gtk_box_pack_end(GTK_BOX(devRow), connBtn, false, false, 5);
  g_signal_connect_data(connBtn, "clicked", G_CALLBACK(handleConnect),
                        new ActionArgs{.manager = manager, .devPath = devPath},
                        (GClosureNotify)FreeActionArgs, (GConnectFlags)0);

  if (station.known) {
    GtkWidget *frgtBtn = gtk_button_new_with_label("Forget");
    gtk_box_pack_end(GTK_BOX(devRow), frgtBtn, false, false, 5);
    g_signal_connect_data(
        frgtBtn, "clicked", G_CALLBACK(handleForget),
        new ActionArgs{.manager = manager, .devPath = devPath},
        (GClosureNotify)FreeActionArgs, (GConnectFlags)0);
  }

  return devRow;
}

void WifiWindow::addTooltip(GtkWidget *widget, const WifiStation &station) {
  std::string tooltip =
      "<b>Security:</b> " + station.type + "\n<b>Signal Strength:</b> ";
  if (station.rssi > -50)
    tooltip += "Excellent";
  else if (station.rssi > -60)
    tooltip += "Good";
  else if (station.rssi > -70)
    tooltip += "Fair";
  else if (station.rssi > -80)
    tooltip += "Weak";
  else
    tooltip += "Very Weak";

  gtk_widget_set_tooltip_markup(widget, tooltip.c_str());
}

void WifiWindow::handleConnect([[maybe_unused]] GtkWidget *widget,
                               gpointer user_data) {
  ActionArgs *args = static_cast<ActionArgs *>(user_data);
  args->manager->Connect(args->devPath);
}

void WifiWindow::handleDisconnect([[maybe_unused]] GtkWidget *widget,
                                  gpointer user_data) {
  WifiWindow *self = static_cast<WifiWindow *>(user_data);
  self->manager->Disconnect();
}

void WifiWindow::handleForget([[maybe_unused]] GtkWidget *widget,
                              gpointer user_data) {
  ActionArgs *args = static_cast<ActionArgs *>(user_data);
  args->manager->Forget(args->devPath);
}

void WifiWindow::handleScan([[maybe_unused]] GtkWidget *widget,
                            gpointer user_data) {
  WifiWindow *self = static_cast<WifiWindow *>(user_data);
  self->manager->Scan();

  if (self->manager->IsScanning()) {
    gtk_widget_set_sensitive(self->scanBtn, false);
  }
}

void WifiWindow::handlePassSubmit([[maybe_unused]] GtkWidget *widget,
                                  gpointer user_data) {
  WifiWindow *self = static_cast<WifiWindow *>(user_data);
  const char *password = gtk_entry_get_text(GTK_ENTRY(self->passEntry));

  if (password && std::strlen(password) > 0) {
    self->manager->SubmitPassphrase(password);
  }
}

void WifiWindow::FreeActionArgs(gpointer data,
                                [[maybe_unused]] GClosure *closure) {
  ActionArgs *args = static_cast<ActionArgs *>(data);
  delete args;
}
