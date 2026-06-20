#include "header/module.hpp"
#include "gtkmm/box.h"
#include <string>

#define TAG "BrightnessModule"

BrightnessModule::BrightnessModule(AppContext *ctx, BrightnessManager *manager)
    : ctx(ctx), manager(manager) {}

Gtk::Box& BrightnessModule::setup() {
  
  auto it = ctx->resStore.icons.find("brightness_base");
  if (it == ctx->resStore.icons.end()) {
    ctx->logger.LogError(TAG,
                         "Icon 'brightness_base' not found in ResourceStore");
    return mainBox;
  }

  // GInputStream *stream = g_memory_input_stream_new_from_data(
  //     it->second.data(), it->second.size(), nullptr);
  // if (!stream) {
  //   ctx->logger.LogError(
  //       TAG, "Failed to create GInputStream for 'brightness_base' icon");
  //   return mainBox;
  // }

  // GError *err = nullptr;
  // GdkPixbuf *pixbuf =
  //     gdk_pixbuf_new_from_stream_at_scale(stream, 18, 18, true, nullptr, &err);

  // GtkWidget *iconWid = gtk_image_new_from_pixbuf(pixbuf);
  // gtk_box_pack_start(GTK_BOX(box), iconWid, false, false, 0);

  mainBox.append(mainLbl);

  update();
  return mainBox;
}

void BrightnessModule::update() {
  short brightness = manager->getLvl();
  if (brightness < 0) {
      mainLbl.set_text("Error");
  } else {
    mainLbl.set_text((std::to_string(brightness) + "%"));
  }
}
