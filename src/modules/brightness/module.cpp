#include "module.hpp"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk/gtk.h"
#include <string>

#define TAG "BrightnessModule"

BrightnessModule::BrightnessModule(AppContext *ctx, BrightnessManager *manager,
                                   BrightnessWindow *window)
    : ctx(ctx), manager(manager), window(window) {}

GtkWidget *BrightnessModule::setup() {
  GtkWidget *evtBox = gtk_event_box_new();
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_container_add(GTK_CONTAINER(evtBox), box);
  g_signal_connect(evtBox, "button-press-event",
                   G_CALLBACK(BrightnessModule::handleWinOpen), this);

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
  gtk_box_pack_start(GTK_BOX(box), mainWid, FALSE, FALSE, 0);

  update();
  return evtBox;
}

void BrightnessModule::update() {
  short brightness = manager->getLvl();
  if (brightness < 0) {
    gtk_label_set_text(GTK_LABEL(mainWid), "Error");
  } else {
    gtk_label_set_text(GTK_LABEL(mainWid),
                       (std::to_string(brightness) + "%").c_str());
  }
}

void BrightnessModule::handleWinOpen(GtkWidget *wid, GdkEventButton *evt,
                                     gpointer data) {
  BrightnessModule *self = static_cast<BrightnessModule *>(data);

  self->ctx->showCtrlWindow("brightness", 300, 50);
}

