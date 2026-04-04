#include "header/module.hpp"
#include "gtk/gtk.h"
#include "modules/screensaver/header/manager.hpp"
#include "services/header/comm_types.hpp"

#define TAG "ScreenSaverModule"

ScreenSaverModule::ScreenSaverModule(AppContext *ctx, CommunicationBus *commBus,
                                     ScreenSaverManager *scrnsavrInstance)
    : screenSaverMgr(scrnsavrInstance), commBus(commBus), logger(&ctx->logger) {}

GtkWidget *ScreenSaverModule::setup() {
  // L is temprorary placeholder until i find suitable icons
  GtkWidget *scrnSvrEBox = gtk_event_box_new();
  btnWid = gtk_label_new("L");
  gtk_container_add(GTK_CONTAINER(scrnSvrEBox), btnWid);
  gtk_widget_set_margin_start(scrnSvrEBox, 10);
  gtk_widget_set_margin_end(scrnSvrEBox, 10);

  g_signal_connect(scrnSvrEBox, "button-press-event",
                   G_CALLBACK(switchScreenSaverCb), this);

  return scrnSvrEBox;
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
