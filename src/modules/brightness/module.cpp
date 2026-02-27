#include "module.hpp"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk-layer-shell.h"
#include "gtk/gtk.h"

#define TAG "BrightnessModule"

BrightnessModule::BrightnessModule(BrightnessManager *manager, AppContext *ctx)
    : manager(manager), ctx(ctx) {}

GtkWidget *BrightnessModule::setup() {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  auto it = ctx->resStore.icons.find("brightness_base");
  if (it == ctx->resStore.icons.end()) {
    ctx->logger.LogError(TAG,
                         "Icon 'brightness_base' not found in ResourceStore");
    return box;
  }
  GInputStream *stream = g_memory_input_stream_new_from_data(
      it->second.data(), it->second.size(), nullptr);
  if (!stream) {
    ctx->logger.LogError(
        TAG, "Failed to create GInputStream for 'brightness_base' icon");
    return box;
  }

  GError *err = nullptr;
  GdkPixbuf *pixbuf =
      gdk_pixbuf_new_from_stream_at_scale(stream, 18, 18, true, nullptr, &err);

  GtkWidget *iconWid = gtk_image_new_from_pixbuf(pixbuf);
  gtk_box_pack_start(GTK_BOX(box), iconWid, FALSE, FALSE, 0);

  mainWid = gtk_label_new(nullptr);
  GtkWidget *evtBox = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(evtBox), mainWid);
  gtk_box_pack_start(GTK_BOX(box), evtBox, FALSE, FALSE, 0);
  g_signal_connect(evtBox, "button-press-event",
                   G_CALLBACK(BrightnessModule::handleWinOpen), this);

  // Window to Control Brightness
  mainWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_layer_init_for_window(GTK_WINDOW(mainWin));

  gtk_layer_set_anchor(GTK_WINDOW(mainWin), GTK_LAYER_SHELL_EDGE_TOP, false);
  gtk_layer_set_anchor(GTK_WINDOW(mainWin), GTK_LAYER_SHELL_EDGE_BOTTOM, false);
  gtk_layer_set_anchor(GTK_WINDOW(mainWin), GTK_LAYER_SHELL_EDGE_LEFT, false);
  gtk_layer_set_anchor(GTK_WINDOW(mainWin), GTK_LAYER_SHELL_EDGE_RIGHT, false);

  gtk_widget_set_size_request(mainWin, 300, 50);
  gtk_widget_set_margin_start(mainWin, 50);
  gtk_widget_set_margin_end(mainWin, 50);
  gtk_widget_set_margin_top(mainWin, 30);
  gtk_widget_set_margin_bottom(mainWin, 30);

  GtkWidget *mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(mainWin), mainBox);

  GtkWidget *titleLbl = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(titleLbl), "<b>Brightness</b>");
  gtk_box_pack_start(GTK_BOX(mainBox), titleLbl, FALSE, FALSE, 0);

  adjWid = gtk_adjustment_new(0, 0, 100, 5, 10, 0);
  GtkWidget* scale =
      gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adjWid);
  gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);
  gtk_box_pack_start(GTK_BOX(mainBox), scale, TRUE, TRUE, 0);
  g_signal_connect(scale, "change-value",
                   G_CALLBACK(BrightnessModule::handleScaleChange), this);

  gtk_widget_show_all(mainWin);
  gtk_widget_hide(mainWin);

  update();
  return box;
}

void BrightnessModule::update() {
  short brightness = manager->getBrightness();
  if (brightness < 0) {
    gtk_label_set_text(GTK_LABEL(mainWid), "Error");
  } else {
    gtk_label_set_text(GTK_LABEL(mainWid),
                       (std::to_string(brightness) + "%").c_str());
    gtk_adjustment_set_value(adjWid, brightness);
  }
}

void BrightnessModule::handleWinOpen(GtkWidget *wid, GdkEventButton* evt,
                                     gpointer data) {
  BrightnessModule *self = static_cast<BrightnessModule *>(data);
  if (!gtk_widget_get_visible(self->mainWin)) {
    self->update();
    gtk_widget_show(self->mainWin);
  }else {
    gtk_widget_hide(self->mainWin);
  }
}

void BrightnessModule::handleScaleChange(GtkRange *range, GtkScrollType *scroll,
                                        gdouble value, gpointer data) {
  BrightnessModule *self = static_cast<BrightnessModule *>(data);

  if (self->manager->setBrightness(static_cast<short>(value))) {
    self->update();
  }
}
