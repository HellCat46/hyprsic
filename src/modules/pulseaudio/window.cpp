#include "header/window.hpp"
#include "gtk/gtk.h"
#include "gtkmm/enums.h"
#include "gtkmm/label.h"
#include "modules/pulseaudio/header/manager.hpp"
#include "services/header/comm_bus.hpp"

#define TAG "PulseAudioWindow"

PulseAudioWindow::PulseAudioWindow(AppContext *ctx, CommunicationBus *commBus,
                                   PulseAudioManager *manager)
    : ctx(ctx), manager(manager), commBus(commBus) {}

void PulseAudioWindow::init() {
  mainBox.set_orientation(Gtk::Orientation::VERTICAL);
  mainBox.set_spacing(10);
  mainBox.set_margin(15);

  // Output Device Controls
  Gtk::Label outTitle;
  outTitle.set_markup("<b>Output Device:</b>");
  outTitle.set_halign(Gtk::Align::START);
  mainBox.append(outTitle);

  Gtk::Box outBox{Gtk::Orientation::HORIZONTAL, 10};
  mainBox.append(outBox);

  outMuteBtn.set_image_from_icon_name("audio-volume-high-symbolic");
  outMuteBtn.signal_clicked().connect([this]() { toggleMute(true); });
  outBox.append(outMuteBtn);

  outScale.set_range(0, 100);
  outBox.append(outScale);
  outScale.signal_change_value().connect(
      [this](Gtk::ScrollType, double value) -> bool {
        handleChgVolume(value, true);
        return false;
      },
      false);

  outDropdown.property_selected().signal_changed().connect(
      [this]() { chgDevice(true); });
  mainBox.append(outDropdown);

  // Input Device Controls
  Gtk::Label inTitle{"<b>Input Device:</b>"};
  inTitle.set_halign(Gtk::Align::START);
  inTitle.set_margin_top(20);
  mainBox.append(inTitle);

  Gtk::Box inBox{Gtk::Orientation::HORIZONTAL, 5};
  mainBox.append(inBox);

  inMuteBtn.set_image_from_icon_name("microphone-sensitivity-high-symbolic");
  inMuteBtn.signal_clicked().connect([this]() { toggleMute(false); });
  inBox.append(inMuteBtn);

  inScale.set_range(0, 100);
  inScale.signal_change_value().connect(
      [this](Gtk::ScrollType, double value) -> bool {
        handleChgVolume(value, false);
        return false;
      },
      false);
  inBox.append(inScale);

  inDropdown.property_selected().signal_changed().connect(
      [this]() { chgDevice(false); });
  mainBox.append(inDropdown);

  ctx->addModule(mainBox, "pulseaudio");
  update();
}

// TODO
void PulseAudioWindow::update() {
  // Adding Items to Output Selector
  // gtk_list_store_clear(outStore);
  GtkTreeIter iter, activeIter;
  bool foundActive = false;
  for (const auto &[devName, devInfo] : manager->outDevs) {
    // gtk_list_store_append(outStore, &iter);

    // gtk_list_store_set(outStore, &iter, 0, devName.c_str(), 1,
    //                    devInfo.description.c_str(), -1);

    if (devName == manager->defOutput) {
      activeIter = iter;
      foundActive = true;

      // Using Default Output Device for Control Widgets
      updateControls(devInfo.mute, true, devInfo.volume, outMuteBtn, outScale);
    }
  }
  if (foundActive) {
    // gtk_combo_box_set_active_iter(GTK_COMBO_BOX(outDropdown), &activeIter);
  }

  // Adding Items to Input Selector
  // gtk_list_store_clear(inStore);
  for (const auto &[devName, devInfo] : manager->inDevs) {
    // gtk_list_store_append(inStore, &iter);

    // gtk_list_store_set(inStore, &iter, 0, devName.c_str(), 1,
    //                    devInfo.description.c_str(), -1);

    if (devName == manager->defInput) {
      activeIter = iter;
      foundActive = true;

      // Using Default Input Device for Control Widgets
      updateControls(devInfo.mute, false, devInfo.volume, inMuteBtn, inScale);
    }
  }

  if (foundActive) {
    // gtk_combo_box_set_active_iter(GTK_COMBO_BOX(inDropdown), &activeIter);
  }
}

void PulseAudioWindow::updateControls(bool mute, bool isOutput,
                                      const std::vector<uint32_t> &volumes,
                                      Gtk::Button &muteBtn, Gtk::Scale &scale) {
  if (isOutput) {
    muteBtn.set_image_from_icon_name(mute ? "audio-volume-muted-symbolic"
                                          : "audio-volume-high-symbolic");
  } else {
    muteBtn.set_image_from_icon_name(
        mute ? "microphone-sensitivity-muted-symbolic"
             : "microphone-sensitivity-high-symbolic");
  }

  uint32_t avgVol = 0;
  for (const auto &vol : volumes) {
    avgVol += vol;
  }
  avgVol /= volumes.size();
  scale.set_value((uint32_t)(((float)avgVol / 65535) * 100));
}

void PulseAudioWindow::handleChgVolume(double value, bool isOutput) {

  if (isOutput) {
    uint32_t volume = (uint32_t)value;

    commBus->SendMessage(
        PASetVolumeRequest{.devName = manager->defOutput,
                           .isOutput = true,
                           .volume = volume,
                           .correlationId = commBus->GetNewCorId()},
        Priority::NORMAL);
  } else {
    uint32_t volume = (uint32_t)value;

    commBus->SendMessage(
        PASetVolumeRequest{.devName = manager->defInput,
                           .isOutput = false,
                           .volume = volume,
                           .correlationId = commBus->GetNewCorId()},
        Priority::NORMAL);
  }
}

void PulseAudioWindow::chgDevice(bool isOutput) {

  // GtkTreeIter iter;
  // if (gtk_combo_box_get_active_iter(combo, &iter)) {
  //   gchar *devName;
  //   gtk_tree_model_get(GTK_TREE_MODEL(gtk_combo_box_get_model(combo)), &iter,
  //   0,
  //                      &devName, -1);

  //   if (combo == GTK_COMBO_BOX(self->outDropdown) &&
  //       self->manager->defOutput != devName) {

  //     self->commBus->SendMessage(
  //         PAUpdateDefDeviceRequest{.devName = devName,
  //                                  .isOutput = true,
  //                                  .correlationId =
  //                                      self->commBus->GetNewCorId()},
  //         Priority::IMMEDIATE);
  //   } else if (combo == GTK_COMBO_BOX(self->inDropdown) &&
  //              self->manager->defInput != devName) {

  //     self->commBus->SendMessage(
  //         PAUpdateDefDeviceRequest{.devName = devName,
  //                                  .isOutput = false,
  //                                  .correlationId =
  //                                      self->commBus->GetNewCorId()},
  //         Priority::IMMEDIATE);
  //   }
  //   g_free(devName);
  // }
}

void PulseAudioWindow::toggleMute(bool isOutput) {
  if (isOutput) {
    commBus->SendMessage(
        PAToggleMuteRequest{.devName = manager->defOutput,
                            .isOutput = true,
                            .correlationId = commBus->GetNewCorId()},
        Priority::IMMEDIATE);
    // TODO
    // if (res == 1) {
    //   if (GDK_IS_PIXBUF(outUnmuteIcon))
    //     gtk_image_set_from_pixbuf(GTK_IMAGE(outIcon), outUnmuteIcon);

    //   ctx->showUpdateWindow(UpdateModule::PULSEAUDIO, "speaker_mute",
    //                         "Output Device Muted");
    // } else if (res == 0) {
    //   if (GDK_IS_PIXBUF(outMuteIcon))
    //     gtk_image_set_from_pixbuf(GTK_IMAGE(outIcon), outMuteIcon);

    //   ctx->showUpdateWindow(UpdateModule::PULSEAUDIO, "speaker_unmute",
    //                         "Output Device Unmuted");
    // }
  } else {
    commBus->SendMessage(
        PAToggleMuteRequest{.devName = manager->defInput,
                            .isOutput = false,
                            .correlationId = commBus->GetNewCorId()},
        Priority::IMMEDIATE);

    // TODO
    // if (res == 1) {
    //   if (GDK_IS_PIXBUF(inUnmuteIcon))
    //     gtk_image_set_from_pixbuf(GTK_IMAGE(inIcon), inUnmuteIcon);

    //   ctx->showUpdateWindow(UpdateModule::PULSEAUDIO, "mic_mute",
    //                         "Input Device Muted");
    // } else if (res == 0) {
    //   if (GDK_IS_PIXBUF(inMuteIcon))
    //     gtk_image_set_from_pixbuf(GTK_IMAGE(inIcon), inMuteIcon);

    //   ctx->showUpdateWindow(UpdateModule::PULSEAUDIO, "mic_unmute",
    //                         "Input Device Unmuted");
    // }
  }
}
