#include "header/window.hpp"
#include "giomm/liststore.h"
#include "gtk/gtkshortcut.h"
#include "gtkmm/enums.h"
#include "gtkmm/label.h"
#include "gtkmm/object.h"
#include "modules/pulseaudio/header/manager.hpp"
#include "services/header/comm_bus.hpp"
#include "sigc++/functors/mem_fun.h"
#include <memory>
#include <string>
#include <vector>

#define TAG "PulseAudioWindow"

PulseAudioWindow::PulseAudioWindow(AppContext *ctx, CommunicationBus *commBus,
                                   PulseAudioManager *manager)
    : ctx(ctx), manager(manager), commBus(commBus) {

  outStore = Gio::ListStore<PulseAudioDeviceObject>::create();
  inStore = Gio::ListStore<PulseAudioDeviceObject>::create();

  devFactory = Gtk::SignalListItemFactory::create();
  devFactory->signal_setup().connect(
      sigc::mem_fun(*this, &PulseAudioWindow::onFactorySetup));
  devFactory->signal_bind().connect(
      sigc::mem_fun(*this, &PulseAudioWindow::onFactoryBind));
}

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
  outScale.set_hexpand(true);
  outBox.append(outScale);
  outScale.signal_change_value().connect(
      [this](Gtk::ScrollType, double value) -> bool {
        handleChgVolume(value, true);
        return false;
      },
      false);

  outPropChgConn = outDropdown.property_selected().signal_changed().connect(
      [this]() { chgDevice(true); });
  outDropdown.set_model(outStore);
  outDropdown.set_factory(devFactory);
  mainBox.append(outDropdown);

  // Input Device Controls
  Gtk::Label inTitle;
  inTitle.set_markup("<b>Input Device:</b>");
  inTitle.set_halign(Gtk::Align::START);
  inTitle.set_margin_top(20);
  mainBox.append(inTitle);

  Gtk::Box inBox{Gtk::Orientation::HORIZONTAL, 5};
  mainBox.append(inBox);

  inMuteBtn.set_image_from_icon_name("microphone-sensitivity-high-symbolic");
  inMuteBtn.signal_clicked().connect([this]() { toggleMute(false); });
  inBox.append(inMuteBtn);

  inScale.set_range(0, 100);
  inScale.set_hexpand(true);
  inScale.signal_change_value().connect(
      [this](Gtk::ScrollType, double value) -> bool {
        handleChgVolume(value, false);
        return false;
      },
      false);
  inBox.append(inScale);

  inPropChgConn = inDropdown.property_selected().signal_changed().connect(
      [this]() { chgDevice(false); });
  inDropdown.set_model(inStore);
  inDropdown.set_factory(devFactory);
  mainBox.append(inDropdown);

  ctx->addModule(mainBox, "pulseaudio");
  update();
}

void PulseAudioWindow::update() {
  
  outPropChgConn.block();
  int idx = 0;
  outStore->remove_all();
  for (const auto &[devName, devInfo] : manager->outDevs) {
    outStore->append(PulseAudioDeviceObject::create(devName, devInfo));

    if (devName == manager->defOutput) {
      outDropdown.set_selected(idx);

      // Using Default Output Device for Control Widgets
      updateControls(devInfo.mute, true, devInfo.volume, outMuteBtn, outScale);
    }

    idx++;
  }
  outPropChgConn.unblock();

  // Adding Items to Input Selector
  inPropChgConn.block();
  idx = 0;
  inStore->remove_all();
  for (const auto &[devName, devInfo] : manager->inDevs) {
    inStore->append(PulseAudioDeviceObject::create(devName, devInfo));

    if (devName == manager->defInput) {
      inDropdown.set_selected(idx);

      // Using Default Input Device for Control Widgets
      updateControls(devInfo.mute, false, devInfo.volume, inMuteBtn, inScale);
    }
    idx++;
  }
  inPropChgConn.unblock();
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

  scale.set_value(((float)avgVol / 65535) * 100);
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

  if (isOutput) {
    auto selected = outDropdown.get_selected();
    if (selected == GTK_INVALID_LIST_POSITION)
      return;

    auto selectedItem = outStore->get_item(selected);
    if (!selectedItem)
      return;

    commBus->SendMessage(
        PAUpdateDefDeviceRequest{.devName = selectedItem->devName,
                                 .isOutput = true,
                                 .correlationId = commBus->GetNewCorId()},
        Priority::IMMEDIATE);
  } else {
    auto selected = inDropdown.get_selected();
    if (selected == GTK_INVALID_LIST_POSITION)
      return;

    auto selectedItem = inStore->get_item(selected);
    if (!selectedItem)
      return;

    commBus->SendMessage(
        PAUpdateDefDeviceRequest{.devName = selectedItem->devName,
                                 .isOutput = false,
                                 .correlationId = commBus->GetNewCorId()},
        Priority::IMMEDIATE);
  }
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

Glib::RefPtr<PulseAudioDeviceObject>
PulseAudioDeviceObject::create(const std::string &devName,
                               const PulseAudioDevice &data) {
  return Glib::make_refptr_for_instance<PulseAudioDeviceObject>(
      new PulseAudioDeviceObject(devName, data));
}

void PulseAudioWindow::onFactorySetup(
    const Glib::RefPtr<Gtk::ListItem> &list_item) {
  auto lbl = Gtk::make_managed<Gtk::Label>("");
  lbl->set_halign(Gtk::Align::START);
  list_item->set_child(*lbl);
}

void PulseAudioWindow::onFactoryBind(
    const Glib::RefPtr<Gtk::ListItem> &list_item) {
  auto lbl = dynamic_cast<Gtk::Label *>(list_item->get_child());
  auto dev =
      std::dynamic_pointer_cast<PulseAudioDeviceObject>(list_item->get_item());
  if (lbl && dev) {
    lbl->set_label(dev->data.description);
  }
}
