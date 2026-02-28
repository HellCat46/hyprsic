#include "module.hpp"

#define TAG "BluetoothModule"

BluetoothModule::BluetoothModule(AppContext *ctx, BluetoothManager *manager,
                                 BluetoothWindow *window)
    : ctx(ctx), manager(manager), window(window) {}

GtkWidget *BluetoothModule::setup() {
  GtkWidget *scanEBox = gtk_event_box_new();
  GtkWidget *img = gtk_label_new("");
  gtk_container_add(GTK_CONTAINER(scanEBox), img);
  gtk_widget_set_margin_start(scanEBox, 10);

  g_signal_connect(scanEBox, "button-press-event",
                   G_CALLBACK(BluetoothModule::switchVisibilityBTMenu), this);

  return scanEBox;
}


void BluetoothModule::switchVisibilityBTMenu(GtkWidget *widget, GdkEvent *e,
                                             gpointer user_data) {
  BluetoothModule *self = static_cast<BluetoothModule *>(user_data);

  self->window->update();
  self->ctx->showCtrlWindow("bluetooth", 400, 200);
}
