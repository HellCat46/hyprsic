#include "header/module.hpp"
#include "gtkmm/label.h"

#define TAG "NotificationModule"

NotificationModule::NotificationModule(AppContext *ctx,
                                       CommunicationBus *commBus,
                                       NotificationManager *notifInstance)
    : manager(notifInstance), ctx(ctx), commBus(commBus) {}

Gtk::Label &NotificationModule::setup() {
  mainLbl.set_text("");

  return mainLbl;
}
