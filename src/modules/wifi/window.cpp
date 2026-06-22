#include "header/window.hpp"
#include "gtkmm/button.h"
#include "gtkmm/enums.h"
#include "gtkmm/label.h"
#include "gtkmm/scrolledwindow.h"
#include "gtkmm/switch.h"
#include "modules/wifi/header/manager.hpp"
#include "services/header/comm_bus.hpp"
#include "services/header/comm_types.hpp"
#include "sigc++/functors/mem_fun.h"
#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#define TAG "WifiWindow"

WifiWindow::WifiWindow(AppContext *ctx, CommunicationBus *commBus,
                       WifiManager *mgr)
    : ctx(ctx), manager(mgr), commBus(commBus) {}

void WifiWindow::init() {
  mainBox.set_orientation(Gtk::Orientation::VERTICAL);
  mainBox.set_spacing(10);
  mainBox.set_margin(10);

  // Top Bar with Title and Scan Button
  Gtk::Box topbar{Gtk::Orientation::HORIZONTAL, 5};
  mainBox.append(topbar);

  Gtk::Label title;
  title.set_markup("<big><b><i>Wi-Fi Networks</i></b></big>");
  title.set_halign(Gtk::Align::START);
  title.set_hexpand(true);
  topbar.append(title);

  scanBtn.set_label("Scan");
  topbar.append(scanBtn);
  scanBtn.signal_clicked().connect(
      sigc::mem_fun(*this, &WifiWindow::handleScan));

  // Power Control
  Gtk::Box powerBox{Gtk::Orientation::HORIZONTAL, 5};
  mainBox.append(powerBox);

  Gtk::Label powerLbl;
  powerLbl.set_markup("<b>Power</b>");
  powerLbl.set_halign(Gtk::Align::START);
  powerLbl.set_hexpand(true);
  powerBox.append(powerLbl);

  Gtk::Switch powerBtn;
  powerBtn.set_active(manager->IsPowered());
  powerBox.append(powerBtn);

  // Connected Device Box
  Gtk::Box connDevBox{Gtk::Orientation::VERTICAL, 5};
  mainBox.append(connDevBox);
  connDevBox.set_margin_top(20);


  Gtk::Label connDevTitle;
  connDevTitle.set_markup("<u>Connected Network:</u>");
  connDevTitle.set_halign(Gtk::Align::START);
  connDevBox.append(connDevTitle);
  connDevBox.append(connDevIBox);

  connDeviceName.set_halign(Gtk::Align::START);
  connDevIBox.append(connDeviceName);

  frgtBtn.set_label("Forget");
  frgtBtn.signal_clicked().connect([this](){
      handleForget(manager->getConnDev());
  });
  connDevIBox.append(frgtBtn);

  disCBtn.set_label("Disconnect");
  disCBtn.signal_clicked().connect(
      sigc::mem_fun(*this, &WifiWindow::handleDisconnect));
  connDevIBox.append(disCBtn);

  // Available Networks
  devBox.set_spacing(5);
  mainBox.append(devBox);
  devBox.set_margin_top(20);


  Gtk::Label availDevTitle;
  availDevTitle.set_markup("<u>Available Networks:</u>");
  availDevTitle.set_halign(Gtk::Align::START);
  devBox.append(availDevTitle);

  Gtk::ScrolledWindow devListScrlBox;
  devListScrlBox.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
  devListScrlBox.set_size_request(400, 200);
  devBox.append(devListScrlBox);

  Gtk::Box devListBox;
  devListBox.set_orientation(Gtk::Orientation::VERTICAL);
  devListBox.set_spacing(5);
  devListScrlBox.set_child(devListBox);

  // Passphrase Input Box
  passEntBox.set_spacing(5);
  devBox.append(passEntBox);

  passEntry.set_visibility(false);
  passEntry.set_placeholder_text("Enter Password");
  passEntry.signal_activate().connect(
      sigc::mem_fun(*this, &WifiWindow::handlePassSubmit));
  passEntBox.append(passEntry);


  connDevBox.hide();
  devBox.hide();
  passEntBox.hide();

  
  ctx->addModule(mainBox, "wifi");

  update();
}

void WifiWindow::update() {
  updateConnDev();

  if (!manager->IsScanning()) {
    scanBtn.set_sensitive(true);
  }

  if (!manager->getAuthDev().empty()) {
    passEntBox.show();
  } else {
    passEntBox.hide();
  }

  
  while (auto child = devListBox.get_first_child()) {
    devListBox.remove(*child);
  }

  std::vector<std::pair<std::string, WifiStation>> devWids;
  for (const auto &[devPath, station] : manager->devices) {
    if (devPath == manager->getConnDev())
      continue;

    devWids.push_back({devPath, station});
  }

  // Sort devices by signal strength
  std::ranges::sort(devWids, std::ranges::greater{},
                    [](const std::pair<std::string, WifiStation> &p) {
                      return p.second.rssi;
                    });
  for (const auto &[devPath, widget] : devWids) {
    Gtk::Box devRow{Gtk::Orientation::HORIZONTAL, 5};
    addDevList(devRow, devPath, widget);
    devListBox.append(devRow);
  }

  if (devWids.size() > 0) {
    devListBox.show();
  } else {
    devListBox.hide();
  }
}

void WifiWindow::updateConnDev() {

  auto it = manager->devices.find(manager->getConnDev());
  if (it != manager->devices.end()) {
      
    WifiStation station = it->second;
    connDeviceName.set_markup("<b>" + station.ssid + "</b>");
    addTooltip(connDeviceName, station);

    connDevBox.show();
    connDevIBox.show();
    connDeviceName.show();

    // ctx->logger.LogInfo(TAG, std::to_string(connDevBox.is_visible()) + " " + std::to_string(connDevIBox.is_visible()) + " " + std::to_string(connDeviceName.is_visible()));
  } else {
    connDevBox.hide();
  }
}

void WifiWindow::addDevList(Gtk::Box &devRow, const std::string &devPath,
                            const WifiStation &station) {

  Gtk::Label devName;
  devName.set_markup("<b>" + station.ssid + "</b>");
  addTooltip(devName, station);
  devName.set_halign(Gtk::Align::START);
  devRow.append(devName);

  Gtk::Button connBtn{"Connect"};
  connBtn.set_halign(Gtk::Align::END);
  devRow.append(connBtn);
  connBtn.signal_clicked().connect([this, devPath] { handleConnect(devPath); });

  if (station.known) {
    Gtk::Button frgtBtn{"Forget"};
    devRow.append(frgtBtn);
    frgtBtn.signal_clicked().connect(
        [this, devPath] { handleForget(devPath); });
  }
}

void WifiWindow::addTooltip(Gtk::Widget &widget, const WifiStation &station) {
  std::string tooltip =
      "<b>Security:</b> " + station.type + "\n<b>Signal Strength:</b> ";
  if (station.rssi >= -50)
    tooltip += "Excellent";
  else if (station.rssi >= -60)
    tooltip += "Good";
  else if (station.rssi >= -70)
    tooltip += "Ok";
  else if (station.rssi >= -80)
    tooltip += "Weak";
  else
    tooltip += "Very Weak";

  widget.set_tooltip_markup(tooltip);
}

void WifiWindow::handleConnect(std::string devPath) {
  commBus->SendMessage(
      WifiConnectRequest{.netPath = devPath,
                         .correlationId = commBus->GetNewCorId()},
      Priority::NORMAL);
}

void WifiWindow::handleDisconnect() {
  // I think It should be immediate??? My Mental Image of this is just
  // dramatically pulling out ethernet cable for some reason.
  commBus->SendMessage(
      WifiDisconnectRequest{.correlationId = commBus->GetNewCorId()},
      Priority::IMMEDIATE);
}

void WifiWindow::handleForget(std::string devPath) {
  commBus->SendMessage(
      WifiForgetRequest{.netPath = devPath,
                        .correlationId = commBus->GetNewCorId()},
      Priority::NORMAL);
}

void WifiWindow::handleScan() {
  commBus->SendMessage(WifiScanRequest{.correlationId = commBus->GetNewCorId()},
                       Priority::HIGH);

  if (manager->IsScanning()) {
    scanBtn.set_sensitive(false);
  }
}

void WifiWindow::handlePassSubmit() {
  std::string password = passEntry.get_text();

  if (!password.empty()) {
    commBus->SendMessage(
        WifiSubmitPassphraseRequest{.password = password,
                                    .correlationId = commBus->GetNewCorId()},
        Priority::IMMEDIATE);
  }
}
