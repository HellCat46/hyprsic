#include "header/module.hpp"
#include "gtkmm/label.h"
#include "modules/screensaver/header/manager.hpp"
#include "services/header/comm_types.hpp"

#define TAG "ScreenSaverModule"

ScreenSaverModule::ScreenSaverModule(AppContext *ctx, CommunicationBus *commBus,
                                     ScreenSaverManager *scrnsavrInstance)
    : screenSaverMgr(scrnsavrInstance), commBus(commBus), logger(&ctx->logger) {}

Gtk::Label& ScreenSaverModule::setup() {
  // L is temprorary placeholder until i find suitable icons
  mainLbl.set_text("L");
  mainLbl.set_margin_start(10);
  mainLbl.set_margin_end(10);
  return mainLbl;
}

void ScreenSaverModule::switchScreenSaverCb([[maybe_unused]] GtkWidget *widget,
                                            [[maybe_unused]] GdkEvent *e,
                                            gpointer user_data) {
  ScreenSaverModule *self = static_cast<ScreenSaverModule *>(user_data);

  if (!self->screenSaverMgr->isActive()) {
      self->commBus->SendMessage(ScrnSvrActivateRequest{.correlationId = self->commBus->GetNewCorId()}, Priority::LOW);
  } else {
      self->commBus->SendMessage(ScrnSvrDeActivateRequest{.correlationId = self->commBus->GetNewCorId()}, Priority::LOW);
   //   gtk_label_set_label(GTK_LABEL(self->btnWid), "L");
    }
}
