#include "app/window.hpp"
#include "glib-object.h"
#include "gtk/gtk.h"
#include "modules/wifi/window.hpp"

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

  GtkWidget *frgtBtn = gtk_button_new_with_label("Forget");
  gtk_box_pack_end(GTK_BOX(connDevIBox), frgtBtn, false, false, 5);
  g_signal_connect(frgtBtn, "clicked", G_CALLBACK(handleForget), this);

  GtkWidget *disCBtn = gtk_button_new_with_label("Disconnect");
  gtk_box_pack_end(GTK_BOX(connDevIBox), disCBtn, false, false,5);
  g_signal_connect(disCBtn, "clicked", G_CALLBACK(handleDisconnect), this);

  // Available Networks
  devBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start(GTK_BOX(mainBox), devBox, false, false, 0);
  gtk_widget_set_margin_top(devBox, 20);

  GtkWidget *availDevTitle = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(availDevTitle), "<u>Available Networks:</u>");
  gtk_widget_set_halign(availDevTitle, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(devBox), availDevTitle, false, false, 0);

  devListScrlBox = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(devListScrlBox),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(devBox), devListScrlBox, false, false, 0);

  gtk_widget_show_all(mainBox);
  gtk_widget_hide(connDevBox);
  gtk_widget_hide(devBox);

  ctx->addModule(mainBox, "wifi");
  
  update();
}

void WifiWindow::update() {
  WifiStation station = manager->ConnectedDevice();
  if (station.connected) {
    gtk_label_set_markup(GTK_LABEL(connDeviceName),
                         ("<b>" + station.ssid + "</b>").c_str());
    gtk_widget_show(connDevBox);
  } else {
    gtk_widget_hide(connDevBox);
  }
  
  if(!manager->IsScanning()){
      gtk_widget_set_sensitive(scanBtn, true);
  }
}

void WifiWindow::handleDisconnect([[maybe_unused]] GtkWidget *widget,
                                  gpointer user_data) {
  WifiWindow *self = static_cast<WifiWindow *>(user_data);
  self->manager->Disconnect();
}

void WifiWindow::handleForget([[maybe_unused]] GtkWidget *widget,
                              gpointer user_data) {
  WifiWindow *self = static_cast<WifiWindow *>(user_data);
  self->manager->Forget(self->manager->ConnectedDevice().ssid);
}

void WifiWindow::handleScan([[maybe_unused]] GtkWidget *widget,
                            gpointer user_data) {
  WifiWindow *self = static_cast<WifiWindow *>(user_data);
  self->manager->Scan();
  
  if(self->manager->IsScanning()){
      gtk_widget_set_sensitive(self->scanBtn, false);
  }
}
