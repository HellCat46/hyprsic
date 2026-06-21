#include "header/module.hpp"

#define TAG "NotificationModule"

NotificationModule::NotificationModule(AppContext *ctx,
                                       CommunicationBus *commBus,
                                       NotificationManager *notifInstance)
    : manager(notifInstance), ctx(ctx),commBus(commBus) {}

Gtk::Box &NotificationModule::setup() {
  mainBox.append(mainIcon);
  mainBox.append(mainLbl);

  update();
  return mainBox;
}

void NotificationModule::update() {
    mainIcon.set_from_icon_name(manager->dnd ? "notifications-disabled-symbolic" : "preferences-system-notifications-symbolic");
    mainLbl.set_text(std::to_string(ctx->dbManager.notifList.size()));
}
