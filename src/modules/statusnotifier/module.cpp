#include "header/module.hpp"
#include "gdk/gdk.h"
#include "gtkmm/enums.h"
#include "gtkmm/gestureclick.h"
#include "gtkmm/label.h"
#include "gtkmm/separator.h"
#include "modules/statusnotifier/header/manager.hpp"
#include <map>
#include <string>

#define TAG "StatusNotifierModule"

StatusNotifierModule::StatusNotifierModule(
    AppContext *ctx, CommunicationBus *commBus,
    StatusNotifierManager *snManagerInstance)
    : logger(&ctx->logger), snManager(snManagerInstance), commBus(commBus) {}

Gtk::Box &StatusNotifierModule::setup() {
  sniBox.set_spacing(2);

  // TODO: Replace with CommunicationBus...
  // snManager->removeCallbacks.push_back(
  //     {StatusNotifierModule::remove, sniBox, &sniApps});

  return sniBox;
}

void StatusNotifierModule::update() {
  for (const auto &[servicePath, appInfo] : snManager->registeredItems) {
    if (sniApps.find(servicePath) != sniApps.end())
      return; // Already Added

    auto &app = sniApps.emplace(servicePath, SNIApp{}).first->second;

    app.icon.set(
        appInfo.pixmap->scale_simple(24, 24, Gdk::InterpType::BILINEAR));

    auto gesClick = Gtk::GestureClick::create();
    gesClick->set_button(GDK_BUTTON_SECONDARY);
    gesClick->signal_pressed().connect(
        [this, servicePath](int, double, double) { handleContextMenuOpen(servicePath); });
    app.icon.add_controller(gesClick);

    app.popOver.set_size_request(200, -1);
    app.popOver.set_margin_bottom(30);

    sniBox.append(app.icon);

    app.parentBox.set_margin_bottom(10);
    app.parentBox.set_margin_top(10);

    app.popOver.set_parent(app.parentBox);
    for (const auto &[index, menuItem] : appInfo.menuActions) {
      if (menuItem.isSeparator) {
        Gtk::Separator sep{Gtk::Orientation::HORIZONTAL};
        app.parentBox.append(sep);
      } else if (menuItem.visible) {
        Gtk::Label menuBtn{menuItem.label};
        auto lblGesClick = Gtk::GestureClick::create();
        menuBtn.set_sensitive(menuItem.enabled);
        menuBtn.add_controller(lblGesClick);

        lblGesClick->set_button(GDK_BUTTON_PRIMARY);
        lblGesClick->signal_pressed().connect(
            [this, servicePath, evtIdx = menuItem.index, lblGesClick](int, double, double) { handleEvtButtonPress(servicePath, evtIdx, lblGesClick->get_current_event_time()); });
        app.parentBox.append(menuBtn);
      }
    }
    app.popOver.popdown();

    sniBox.append(app.icon);
  }
}

void StatusNotifierModule::remove(std::string servicePath) {
  auto it = sniApps.find(servicePath);
  if (it != sniApps.end()) {
    it->second.icon.unparent();
    sniApps.erase(it);
  }
}

void StatusNotifierModule::handleContextMenuOpen(std::string servicePath) {
  auto it = sniApps.find(servicePath);
  if (it == sniApps.end())
    return;

  if (it->second.popOver.is_visible()) {
    it->second.popOver.popdown();
  } else {
    it->second.popOver.popup();
  }
}

void StatusNotifierModule::handleEvtButtonPress(std::string servicePath,
                                                 uint32_t evtIdx,
                                                 uint32_t timestamp) {
  auto item = snManager->registeredItems.find(servicePath);
  if (item == snManager->registeredItems.end())
    return;

  commBus->SendMessage(
      SNIExecuteMenuAction{.itemService = servicePath,
                           .menuPath = item->second.menu_path,
                           .timestamp = timestamp,
                           .actionIndex = evtIdx,
                           .correlationId = commBus->GetNewCorId()},
      Priority::HIGH);
}
