#include "header/module.hpp"
#include "gtkmm/label.h"
#include "modules/wifi/header/window.hpp"
#include <cctype>

#define TAG "WifiModule"

WifiModule::WifiModule(AppContext *ctx, WifiManager *mgr)
    : ctx(ctx), manager(mgr) {}

Gtk::Box &WifiModule::setup() {
  mainBox.set_spacing(5);
  mainBox.append(icon);
  mainBox.append(lbl);

  update();
  return mainBox;
}

void WifiModule::update() {

  auto it = manager->devices.find(manager->getConnDev());
  if (it != manager->devices.end()) {
    lbl.set_text(it->second.ssid);

    WifiWindow::addTooltip(mainBox, it->second);
    std::string tooltipTxt = mainBox.get_tooltip_markup();
    size_t pos = tooltipTxt.rfind(" ");
    if (pos == std::string::npos) {
      return;
    }

    tooltipTxt = tooltipTxt.substr(pos + 1);
    if (!tooltipTxt.empty()) {
      tooltipTxt[0] = std::tolower(tooltipTxt[0]);
      tooltipTxt = "signal-" + tooltipTxt + "-";
    }

    icon.set_from_icon_name("network-wireless-" + tooltipTxt + "symbolic");
  } else {

    icon.set_from_icon_name("network-wireless-offline-symbolic");
    lbl.set_text("Not Connected");
  }
}
