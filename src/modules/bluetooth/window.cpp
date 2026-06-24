#include "header/window.hpp"
#include "gtkmm/box.h"
#include "gtkmm/button.h"
#include "gtkmm/enums.h"
#include "gtkmm/label.h"
#include "gtkmm/listboxrow.h"
#include "gtkmm/object.h"
#include "gtkmm/scrolledwindow.h"
#include "gtkmm/switch.h"
#include "modules/bluetooth/header/manager.hpp"
#include "sigc++/adaptors/bind.h"
#include "sigc++/functors/mem_fun.h"
#define TAG "BluetoothWindow"

BluetoothWindow::BluetoothWindow(AppContext *ctx, CommunicationBus *commBus,
                                 BluetoothManager *manager)
    : ctx(ctx), manager(manager), commBus(commBus) {}

void BluetoothWindow::init() {
  menuBox.set_orientation(Gtk::Orientation::VERTICAL);

  // Navigation Box with Close Button
  Gtk::Box navBox{Gtk::Orientation::HORIZONTAL, 5};
  menuBox.append(navBox);
  navBox.set_margin_bottom(10);

  // Items in Nav Bar
  Gtk::Label title;
  title.set_markup("<big><b>Bluetooth Manager</b></big>");
  title.set_xalign(0);
  title.set_hexpand(true);
  navBox.append(title);

  scanBtn.set_label(manager->discovering ? "Stop" : "Scan");
  navBox.append(scanBtn);
  scanBtn.signal_clicked().connect(
      sigc::mem_fun(*this, &BluetoothWindow::handleDiscovery));

  // Top Box with Power and Scan Buttons
  Gtk::Box topBox{Gtk::Orientation::HORIZONTAL, 5};
  menuBox.append(topBox);

  // Power Toggle
  Gtk::Box powerBox{Gtk::Orientation::HORIZONTAL, 5};
  Gtk::Label powerLbl;
  powerLbl.set_markup("<b>Power</b>");
  powerLbl.set_halign(Gtk::Align::START);
  powerLbl.set_hexpand(true);
  powerBox.append(powerLbl);

  
  
  powerBtn.set_active(manager->power);
  powerBtn.set_state(manager->power);
  powerBtn.signal_state_set().connect(
      [this](bool state) -> bool {
        this->handlePower(state);
        return false;
      },
      false);
  powerBox.append(powerBtn);
  topBox.append(powerBox);

  devBox.set_orientation(Gtk::Orientation::VERTICAL);
  devBox.set_spacing(5);
  menuBox.append(devBox);

  // Device List Sections
  pairedDevTitle.set_markup("<b><u>Paired Devices:</u></b>");
  pairedDevTitle.set_halign(Gtk::Align::START);
  devBox.append(pairedDevTitle);
  devBox.append(pairedDevList);

  availDevTitle.set_markup("<b><u>Available Devices:</u></b>");
  availDevTitle.set_halign(Gtk::Align::START);
  devBox.append(availDevTitle);

  Gtk::ScrolledWindow availDevScroll;
  availDevScroll.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
  availDevScroll.set_size_request(200, 150);

  availDevScroll.set_child(availDevList);
  devBox.append(availDevScroll);

  pairedDevTitle.hide();
  pairedDevList.hide();
  availDevTitle.hide();
  availDevList.hide();

  devBox.set_margin_top(20);
  menuBox.set_margin_top(5);
  menuBox.set_margin_bottom(5);
  menuBox.set_margin_start(10);
  menuBox.set_margin_end(10);

  update(true);
  ctx->addModule(menuBox, "bluetooth");
}

void BluetoothWindow::update(bool force) {
  if (!menuBox.get_visible() && !force)
    return;

  powerBtn.set_active(manager->power);
  powerBtn.set_state(manager->power);

  availDevList.remove_all();
  pairedDevList.remove_all();

  // Repopulate Device List
  int pairedDevs = 0, availDevs = 0;
  auto devices = manager->getDeviceList();

  if (devices.size() > 0) {
    for (auto [_, device] : devices) {

      if (device.paired) {
        addDeviceEntry(device, pairedDevList);
        pairedDevs++;
      } else {
        addDeviceEntry(device, availDevList);
        availDevs++;
      }

      if (availDevs >= 5) {
        break; // Limit to 5 new devices shown
      }
    }
  }

  if (pairedDevs > 0) {
    pairedDevTitle.show();
    pairedDevList.show();
  } else {
    pairedDevTitle.hide();
    pairedDevList.hide();
  }

  if (availDevs > 0) {
    availDevTitle.show();
    availDevList.show();
  } else {
    availDevTitle.hide();
    availDevList.hide();
  }
}

void BluetoothWindow::addDeviceEntry(const Device &dev, Gtk::ListBox &listBox) {
  Gtk::Box devItem{Gtk::Orientation::HORIZONTAL, 5};
  devItem.set_margin_bottom(5);
  devItem.set_margin_top(5);

  // Device Label
  std::string devLblStr = dev.name.length() > 0 ? dev.name : dev.addr;
  if (devLblStr.size() > 20) {
    devLblStr = devLblStr.substr(0, 17) + "...";
  }

  // Tooltip Info
  std::string devTooltip = "Address: " + dev.addr;
  if (dev.batteryPer != -1)
    devTooltip += "\nBattery: " + std::to_string(dev.batteryPer) + "%";
  if (dev.rssi != -110)
    devTooltip += "\nSignal Strength: " + std::to_string(dev.rssi) + " dBm";

  // Adding Label and Tooltip
  Gtk::Label devLbl{devLblStr};
  devLbl.set_xalign(0);
  devLbl.set_tooltip_text(devTooltip);
  devLbl.set_hexpand(true);
  devItem.append(devLbl);

  // Only Shown for Paired Devices
  if (dev.paired) {
    // Device Unpair Button
    auto devRmvBtn = Gtk::make_managed<Gtk::Button>("✖");
    devRmvBtn->set_tooltip_text("Remove Device");
    devItem.append(*devRmvBtn);
    devRmvBtn->signal_clicked().connect(sigc::bind(
        sigc::mem_fun(*this, &BluetoothWindow::handleDeviceRemove), dev.path));

    // Device Trust Button
    auto devTrustBtn = Gtk::make_managed<Gtk::Button>("");
    devTrustBtn->set_tooltip_text(dev.trusted ? "Untrust Device"
                                             : "Trust Device");
    devItem.append(*devTrustBtn);
    devTrustBtn->signal_clicked().connect(
        sigc::bind(sigc::mem_fun(*this, &BluetoothWindow::handleDeviceTrust),
                   !dev.trusted, dev.path));
  }

  // Device Connect Button Connect Icon
  auto devConnBtn = Gtk::make_managed<Gtk::Button>("");
  devConnBtn->set_tooltip_text(dev.connected ? "Disconnect Device"
                                            : "Connect Device");
  devItem.append(*devConnBtn);
  devConnBtn->signal_clicked().connect(
      sigc::bind(sigc::mem_fun(*this, &BluetoothWindow::handleDeviceConnect),
                 !dev.connected, dev.path));

  auto row = Gtk::make_managed<Gtk::ListBoxRow>();
  row->set_child(devItem);

  listBox.append(*row);
}

void BluetoothWindow::handleDiscovery() {

  if (manager->discovering) {
    ctx->logger.LogInfo(TAG, "Stopping Bluetooth Discovery.");

    commBus->SendMessage(BtSwitchDiscoveryRequest{false, ModuleType::BLUETOOTH,
                                                  commBus->GetNewCorId()},
                         Priority::LOW);
  } else {
    ctx->logger.LogInfo(TAG, "Starting Bluetooth Discovery.");

    commBus->SendMessage(BtSwitchDiscoveryRequest{true, ModuleType::BLUETOOTH,
                                                  commBus->GetNewCorId()},
                         Priority::NORMAL);
    // self->update();
  }

  scanBtn.set_label(manager->discovering ? "Stop" : "Scan");
}

void BluetoothWindow::handlePower(bool state) {

  commBus->SendMessage(BtSwitchPowerRequest{state, ModuleType::BLUETOOTH,
                                            commBus->GetNewCorId()},
                       Priority::NORMAL);

  // std::string msg = "Bluetooth Power Switched ";
  // msg += (state ? "ON" : "OFF");
  // self->ctx->logger.LogInfo(TAG, msg);
  // self->ctx->showUpdateWindow(UpdateModule::BLUETOOTH,
  //                             state ? "base" : "disabled", msg);
}

void BluetoothWindow::handleDeviceTrust(bool state, std::string devIfacePath) {

  commBus->SendMessage(BtTrustRequest{state, devIfacePath,
                                      ModuleType::BLUETOOTH,
                                      commBus->GetNewCorId()},
                       Priority::LOW);
}

void BluetoothWindow::handleDeviceRemove(std::string devIfacePath) {

  commBus->SendMessage(BtRemoveRequest{devIfacePath, ModuleType::BLUETOOTH,
                                       commBus->GetNewCorId()},
                       Priority::LOW);
}

void BluetoothWindow::handleDeviceConnect(bool state,
                                          std::string devIfacePath) {

  ctx->logger.LogInfo(TAG, "Device Connect: " + devIfacePath + " state: " + std::to_string(state));
  commBus->SendMessage(BtConnectRequest{state, devIfacePath,
                                        ModuleType::BLUETOOTH,
                                        commBus->GetNewCorId()},
                       Priority::LOW);
}
