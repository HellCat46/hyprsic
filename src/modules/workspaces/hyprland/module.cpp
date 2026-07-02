#include "header/module.hpp"
#include "glibmm/main.h"
#include "glibmm/priorities.h"
#include "gtkmm/box.h"
#include "gtkmm/eventcontrollerscroll.h"
#include "gtkmm/gestureclick.h"
#include "gtkmm/label.h"
#include "modules/workspaces/hyprland/header/manager.hpp"
#include "services/header/comm_bus.hpp"
#include <algorithm>
#include <string>

HyprWSModule::HyprWSModule(AppContext *ctx, CommunicationBus *commBus,
                           HyprWSManager *hyprInstance)
    : hyprInstance(hyprInstance), logger(&ctx->logger), commBus(commBus) {}

Gtk::Box &HyprWSModule::setup(unsigned char monitorId) {

  mainBox.set_spacing(15);
  mainBox.set_hexpand(true);

  wsBox.set_spacing(15);
  wsBox.set_margin_start(15);

  auto wsScroll = Gtk::EventControllerScroll::create();
  wsScroll->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
  wsScroll->signal_scroll().connect(
      [this](double, double dy) -> bool {
        this->handleWSScroll(dy);
        return false;
      },
      false);
  wsBox.add_controller(wsScroll);
  mainBox.append(wsBox);

  spWSBox.set_spacing(15);
  spWSBox.set_margin_start(15);
  mainBox.append(spWSBox);

  this->monitorId = monitorId;
  updateWorkspaces();
  hyprInstance->subscribe([this]() { this->updateWorkspaces(); });

  return mainBox;
}

void HyprWSModule::updateWorkspaces() {

  if (!hyprInstance->GetWorkspaces()) {
    Glib::signal_idle().connect_once([this] { this->updateWorkspaceUI(); },
                                     Glib::PRIORITY_HIGH_IDLE);
  }
}

void HyprWSModule::updateWorkspaceUI() {
  std::string txt = "";

  while (auto child = wsBox.get_first_child()) {
    wsBox.remove(*child);
  }

  while (auto child = spWSBox.get_first_child()) {
    spWSBox.remove(*child);
  }

  for (auto workspace : hyprInstance->workspaces) {
    txt = workspace.second.name;

    if (monitorId != workspace.second.monitorId)
      continue;

    if (hyprInstance->activeWorkspaceId == workspace.first &&
        workspace.first >= 0) {
      txt = "[ " + txt + " ]";
    } else if (workspace.first < 0 && txt.length() > 0) {
      std::transform(txt.begin(), txt.begin() + 1, txt.begin(), ::toupper);
      txt = txt[0];
    }

    txt = "<b>" + txt + "</b>";

    Gtk::Label wsLbl;
    wsLbl.set_markup(txt);

    auto evtBox = Gtk::GestureClick::create();
    wsLbl.add_controller(evtBox);

    if (workspace.first < 0) {
      wsLbl.set_tooltip_markup("<b>" + workspace.second.name + "</b>");
      spWSBox.append(wsLbl);

      evtBox->signal_pressed().connect(
          [this, wsId = workspace.first, wsName = workspace.second.name](int, int, double) {
            chgSPWS(wsId, wsName);
          });
    } else {
      wsBox.append(wsLbl);

      evtBox->signal_pressed().connect(
          [this, wsId = workspace.first](int, int, double) { chgWS(wsId); });
    }
  }
}

void HyprWSModule::chgWS(unsigned int wsId) {
  commBus->SendMessage(
      HyprSwitchWSRequest{
          wsId,
          ModuleType::HYPR,
          commBus->GetNewCorId(),
      },
      Priority::HIGH);
}

void HyprWSModule::handleWSScroll(double dy) {
  if (dy < 0) {
      
    commBus->SendMessage(HyprMoveWSRequest{monitorId, false, ModuleType::HYPR,
                                           commBus->GetNewCorId()},
                         Priority::IMMEDIATE);
  } else if (dy > 0) {
    commBus->SendMessage(HyprMoveWSRequest{monitorId, true, ModuleType::HYPR,
                                           commBus->GetNewCorId()},
                         Priority::IMMEDIATE);
  }
}

void HyprWSModule::chgSPWS(unsigned int id, std::string name) {

  commBus->SendMessage(
      HyprSwitchSPWSRequest{id, name, ModuleType::HYPR, commBus->GetNewCorId()},
      Priority::IMMEDIATE);
}
