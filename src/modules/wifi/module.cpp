#include "header/module.hpp"
#include "gtkmm/label.h"

WifiModule::WifiModule(AppContext *ctx, WifiManager *mgr)
    : ctx(ctx), manager(mgr) {}

Gtk::Label &WifiModule::setup() {
  mainLbl.set_text("Not Connected");
  
  update();
  return mainLbl;
}

void WifiModule::update() {
  auto it = manager->devices.find(manager->getConnDev());
  if (it != manager->devices.end()) {
    mainLbl.set_text(it->second.ssid);
    return;
  }
  mainLbl.set_text("Not Connected");
}
