#include "header/module.hpp"

#define TAG "PulseAudioModule"

PulseAudioModule::PulseAudioModule(PulseAudioManager *paMgr, AppContext *ctx)
    : manager(paMgr), ctx(ctx), setupComp(false) {}

Gtk::Box& PulseAudioModule::setup() {
    menuBox.set_spacing(10);
  barInIcon.set_from_icon_name("microphone-sensitivity-high-symbolic");
  menuBox.append(barInIcon);

  barOutIcon.set_from_icon_name("audio-volume-high-symbolic");
  menuBox.append(barOutIcon);

  setupComp = true;
  return menuBox;
}

void PulseAudioModule::update() {
  if (!setupComp)
    return;

  auto it = manager->outDevs.find(manager->defOutput);
  if (it != manager->outDevs.end()) {
    barOutIcon.set_from_icon_name(it->second.mute ? "audio-volume-muted-symbolic" : "audio-volume-high-symbolic");
  }

  it = manager->inDevs.find(manager->defInput);
  if (it != manager->inDevs.end()) {
    barInIcon.set_from_icon_name(it->second.mute ? "microphone-sensitivity-muted-symbolic" : "microphone-sensitivity-high-symbolic");
  }
}
