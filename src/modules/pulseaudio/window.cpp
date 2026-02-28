#include "window.hpp"
#include "gtk/gtk.h"
#include "manager.hpp"

#define TAG "PulseAudioWindow"

PulseAudioWindow::PulseAudioWindow(AppContext *ctx, PulseAudioManager *manager)
    : manager(manager), ctx(ctx) {}

void PulseAudioWindow::init() {
  GtkWidget *mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(mainBox, 15);
  gtk_widget_set_margin_end(mainBox, 15);
  gtk_widget_set_margin_top(mainBox, 15);
  gtk_widget_set_margin_bottom(mainBox, 15);

  // Output Device Controls
  GtkWidget *outTitle = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(outTitle), "<b>Output Device:</b>");
  gtk_widget_set_halign(outTitle, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(mainBox), outTitle, FALSE, FALSE, 0);

  GtkWidget *outBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(mainBox), outBox, FALSE, FALSE, 0);

  outMuteBtn = gtk_event_box_new();
  gtk_box_pack_start(GTK_BOX(outBox), outMuteBtn, FALSE, FALSE, 0);
  g_signal_connect(outMuteBtn, "button-press-event",
                   G_CALLBACK(handleToggleMute), this);
  outIcon = gtk_image_new_from_pixbuf(nullptr);
  gtk_container_add(GTK_CONTAINER(outMuteBtn), outIcon);

  if (GDK_IS_PIXBUF(outUnmuteIcon))
    gtk_image_set_from_pixbuf(GTK_IMAGE(outIcon), outUnmuteIcon);

  outScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_box_pack_start(GTK_BOX(outBox), outScale, TRUE, TRUE, 0);
  g_signal_connect(outScale, "change-value", G_CALLBACK(handleChgVolume), this);

  outStore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  outDropdown = gtk_combo_box_new_with_model(GTK_TREE_MODEL(outStore));
  g_signal_connect(outDropdown, "changed", G_CALLBACK(chgDevice), this);
  gtk_box_pack_start(GTK_BOX(mainBox), outDropdown, FALSE, FALSE, 0);

  GtkCellRenderer *outRenderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(outDropdown), outRenderer, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(outDropdown), outRenderer,
                                 "text", 1, nullptr);

  // Input Device Controls
  GtkWidget *inTitle = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(inTitle), "<b>Input Device:</b>");
  gtk_widget_set_halign(inTitle, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(mainBox), inTitle, FALSE, FALSE, 0);
  gtk_widget_set_margin_top(inTitle, 20);

  GtkWidget *inBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(mainBox), inBox, TRUE, TRUE, 0);

  inMuteBtn = gtk_event_box_new();
  gtk_box_pack_start(GTK_BOX(inBox), inMuteBtn, FALSE, FALSE, 0);
  g_signal_connect(inMuteBtn, "button-press-event",
                   G_CALLBACK(handleToggleMute), this);
  inIcon = gtk_image_new_from_pixbuf(nullptr);
  gtk_container_add(GTK_CONTAINER(inMuteBtn), inIcon);

  if (GDK_IS_PIXBUF(inUnmuteIcon))
    gtk_image_set_from_pixbuf(GTK_IMAGE(inIcon), inUnmuteIcon);

  inScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_box_pack_start(GTK_BOX(inBox), inScale, TRUE, TRUE, 0);
  g_signal_connect(inScale, "change-value", G_CALLBACK(handleChgVolume), this);

  inStore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  inDropdown = gtk_combo_box_new_with_model(GTK_TREE_MODEL(inStore));
  g_signal_connect(inDropdown, "changed", G_CALLBACK(chgDevice), this);
  gtk_box_pack_start(GTK_BOX(mainBox), inDropdown, FALSE, FALSE, 0);

  GtkCellRenderer *inRenderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(inDropdown), inRenderer, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(inDropdown), inRenderer,
                                 "text", 1, nullptr);

  gtk_widget_show_all(mainBox);

  ctx->addModule(mainBox, "pulseaudio");
  update();
}

void PulseAudioWindow::setupIcons() {
  // Load Svg Icons
  GInputStream *stream;
  GError *err;
  do {
    stream = nullptr;
    err = nullptr;
    inMuteIcon = nullptr;
    auto icon = ctx->resStore.icons.find("audio_mic_mute");
    if (icon == ctx->resStore.icons.end()) {
      ctx->logger.LogError(TAG, "Mic Mute Icon Not Found in Resource Store");
      break;
    }

    stream = g_memory_input_stream_new_from_data(icon->second.data(),
                                                 icon->second.size(), nullptr);
    if (stream == nullptr) {
      ctx->logger.LogError(TAG,
                           "Failed to Create GInputStream for Mic Mute Icon");
      break;
    }

    inMuteIcon = gdk_pixbuf_new_from_stream_at_scale(stream, 16, 16, TRUE,
                                                     nullptr, &err);
    if (err) {
      ctx->logger.LogError(TAG, "Failed to load mic mute icon: " +
                                    std::string(err->message));
      g_error_free(err);
      break;
    }
    g_object_unref(stream);
  } while (0);

  do {
    stream = nullptr;
    err = nullptr;
    inUnmuteIcon = nullptr;
    // Unmute Icon
    auto icon = ctx->resStore.icons.find("audio_mic_unmute");
    if (icon == ctx->resStore.icons.end()) {
      ctx->logger.LogError(TAG, "Mic Unmute Icon Not Found in Resource Store");
      break;
    }

    stream = g_memory_input_stream_new_from_data(icon->second.data(),
                                                 icon->second.size(), nullptr);
    if (stream == nullptr) {
      ctx->logger.LogError(TAG,
                           "Failed to Create GInputStream for Mic Unmute Icon");
      break;
    }

    inUnmuteIcon = gdk_pixbuf_new_from_stream_at_scale(stream, 16, 16, TRUE,
                                                       nullptr, &err);
    if (err) {
      ctx->logger.LogError(TAG, "Failed to load mic unmute icon: " +
                                    std::string(err->message));
      g_error_free(err);
      break;
    }
    g_object_unref(stream);
  } while (0);

  do {
    stream = nullptr;
    err = nullptr;
    outMuteIcon = nullptr;
    auto icon = ctx->resStore.icons.find("audio_speaker_mute");
    if (icon == ctx->resStore.icons.end()) {
      ctx->logger.LogError(TAG,
                           "Speaker Mute Icon Not Found in Resource Store");
      break;
    }

    stream = g_memory_input_stream_new_from_data(icon->second.data(),
                                                 icon->second.size(), nullptr);
    if (stream == nullptr) {
      ctx->logger.LogError(
          TAG, "Failed to Create GInputStream for Speaker Mute Icon");
      break;
    }

    outMuteIcon = gdk_pixbuf_new_from_stream_at_scale(stream, 16, 16, TRUE,
                                                      nullptr, &err);
    if (err) {
      ctx->logger.LogError(TAG, "Failed to load volume mute icon: " +
                                    std::string(err->message));
      g_error_free(err);
      break;
    }
    g_object_unref(stream);
  } while (0);

  do {
    stream = nullptr;
    err = nullptr;
    outUnmuteIcon = nullptr;
    auto icon = ctx->resStore.icons.find("audio_speaker_unmute");
    if (icon == ctx->resStore.icons.end()) {
      ctx->logger.LogError(TAG,
                           "Speaker Unmute Icon Not Found in Resource Store");
      break;
    }

    stream = g_memory_input_stream_new_from_data(icon->second.data(),
                                                 icon->second.size(), nullptr);
    if (stream == nullptr) {
      ctx->logger.LogError(
          TAG, "Failed to Create GInputStream for Speaker Unmute Icon");
      break;
    }

    outUnmuteIcon = gdk_pixbuf_new_from_stream_at_scale(stream, 16, 16, TRUE,
                                                        nullptr, &err);
    if (err) {
      ctx->logger.LogError(TAG, "Failed to load volume unmute icon: " +
                                    std::string(err->message));
      g_error_free(err);
      break;
    }
    g_object_unref(stream);
  } while (0);

  ctx->logger.LogInfo(
      TAG,
      "Icons Loaded: " + std::string(GDK_IS_PIXBUF(inMuteIcon) ? "Yes" : "No") +
          ", " + std::string(GDK_IS_PIXBUF(inUnmuteIcon) ? "Yes" : "No") +
          ", " + std::string(GDK_IS_PIXBUF(outMuteIcon) ? "Yes" : "No") + ", " +
          std::string(GDK_IS_PIXBUF(outUnmuteIcon) ? "Yes" : "No"));
}

void PulseAudioWindow::update() {
  // Adding Items to Output Selector
  gtk_list_store_clear(outStore);
  GtkTreeIter iter, activeIter;
  bool foundActive = false;
  for (const auto &[devName, devInfo] : manager->outDevs) {
    gtk_list_store_append(outStore, &iter);

    gtk_list_store_set(outStore, &iter, 0, devName.c_str(), 1,
                       devInfo.description.c_str(), -1);

    if (devName == manager->defOutput) {
      activeIter = iter;
      foundActive = true;

      // Using Default Output Device for Control Widgets
      updateControls(devInfo.mute, devInfo.volume, outIcon, outScale);
    }
  }
  if (foundActive) {
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(outDropdown), &activeIter);
  }

  // Adding Items to Input Selector
  gtk_list_store_clear(inStore);
  for (const auto &[devName, devInfo] : manager->inDevs) {
    gtk_list_store_append(inStore, &iter);

    gtk_list_store_set(inStore, &iter, 0, devName.c_str(), 1,
                       devInfo.description.c_str(), -1);

    if (devName == manager->defInput) {
      activeIter = iter;
      foundActive = true;

      // Using Default Input Device for Control Widgets
      updateControls(devInfo.mute, devInfo.volume, inIcon, inScale);
    }
  }

  if (foundActive) {
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(inDropdown), &activeIter);
  }
}

void PulseAudioWindow::updateControls(bool mute,
                                      const std::vector<uint32_t> &volume,
                                      GtkWidget *icon, GtkWidget *scale) {
  if (icon == outIcon && GDK_IS_PIXBUF(outUnmuteIcon) &&
      GDK_IS_PIXBUF(outMuteIcon)) {
    gtk_image_set_from_pixbuf(GTK_IMAGE(icon),
                              mute ? outUnmuteIcon : outMuteIcon);

  } else if (icon == inIcon && GDK_IS_PIXBUF(inUnmuteIcon) &&
             GDK_IS_PIXBUF(inMuteIcon)) {
    gtk_image_set_from_pixbuf(GTK_IMAGE(icon),
                              mute ? inUnmuteIcon : inMuteIcon);
  }

  uint32_t avgVol = 0;
  for (const auto &vol : volume) {
    avgVol += vol;
  }
  avgVol /= volume.size();
  gtk_range_set_value(GTK_RANGE(scale),
                      (uint32_t)(((float)avgVol / 65535) * 100));
}

void PulseAudioWindow::handleChgVolume(GtkRange *range, GtkScrollType *scroll,
                                       gdouble value, gpointer data) {
  PulseAudioWindow *self = static_cast<PulseAudioWindow *>(data);

  if (range == GTK_RANGE(self->outScale)) {
    uint32_t volume = (uint32_t)gtk_range_get_value(range);
    self->manager->setVolume(self->manager->defOutput, true, volume);
  } else if (range == GTK_RANGE(self->inScale)) {
    uint32_t volume = (uint32_t)gtk_range_get_value(range);
    self->manager->setVolume(self->manager->defInput, false, volume);
  }
}

void PulseAudioWindow::chgDevice(GtkComboBox *combo, gpointer data) {
  PulseAudioWindow *self = static_cast<PulseAudioWindow *>(data);

  GtkTreeIter iter;
  if (gtk_combo_box_get_active_iter(combo, &iter)) {
    gchar *devName;
    gtk_tree_model_get(GTK_TREE_MODEL(gtk_combo_box_get_model(combo)), &iter, 0,
                       &devName, -1);

    self->manager->updateDefDevice(std::string(devName),
                                   combo == GTK_COMBO_BOX(self->outDropdown));

    g_free(devName);
  }
}

void PulseAudioWindow::handleToggleMute(GtkWidget *widget, gpointer data) {
  PulseAudioWindow *self = static_cast<PulseAudioWindow *>(data);

  if (widget == self->outMuteBtn) {
    self->toggleMute(widget, data, true);
  } else if (widget == self->inMuteBtn) {
    self->toggleMute(widget, data, false);
  }
}

void PulseAudioWindow::toggleMute(GtkWidget *widget, gpointer data,
                                  bool isOutput) {
  if (isOutput) {
    short res = manager->toggleMute(manager->defOutput, true);
    if (res == 1) {
      if (GDK_IS_PIXBUF(outUnmuteIcon))
        gtk_image_set_from_pixbuf(GTK_IMAGE(outIcon), outUnmuteIcon);

      ctx->showUpdateWindow(UpdateModule::PULSEAUDIO, "speaker_mute",
                            "Output Device Muted");
    } else if (res == 0) {
      if (GDK_IS_PIXBUF(outMuteIcon))
        gtk_image_set_from_pixbuf(GTK_IMAGE(outIcon), outMuteIcon);

      ctx->showUpdateWindow(UpdateModule::PULSEAUDIO, "speaker_unmute",
                            "Output Device Unmuted");
    }
  } else {
    short res = manager->toggleMute(manager->defInput, false);
    if (res == 1) {
      if (GDK_IS_PIXBUF(inUnmuteIcon))
        gtk_image_set_from_pixbuf(GTK_IMAGE(inIcon), inUnmuteIcon);

      ctx->showUpdateWindow(UpdateModule::PULSEAUDIO, "mic_mute",
                            "Input Device Muted");
    } else if (res == 0) {
      if (GDK_IS_PIXBUF(inMuteIcon))
        gtk_image_set_from_pixbuf(GTK_IMAGE(inIcon), inMuteIcon);

      ctx->showUpdateWindow(UpdateModule::PULSEAUDIO, "mic_unmute",
                            "Input Device Unmuted");
    }
  }
}
