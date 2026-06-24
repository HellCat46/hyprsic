#include "header/module.hpp"
#include "gtkmm/box.h"
#include <string>

#define TAG "BrightnessModule"

BrightnessModule::BrightnessModule(AppContext *ctx, BrightnessManager *manager)
    : ctx(ctx), manager(manager) {}

Gtk::Box &BrightnessModule::setup() {
  mainImg.set_from_icon_name("display-brightness-symbolic");
  mainBox.append(mainImg);
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
