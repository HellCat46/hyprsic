#include "module.hpp"
#include "gtk/gtk.h"

#define TAG "ScreenSaverModule"

ScreenSaverModule::ScreenSaverModule(AppContext *ctx,
                                     ScreenSaverManager *scrnsavrInstance) {
  screenSaverMgr = scrnsavrInstance;
  screenSaverMgr = new ScreenSaverManager(ctx);
  logger = &ctx->logging;
}

void ScreenSaverModule::setup(GtkWidget *grid) {
  // L is temprorary placeholder until i find suitable icons
  GtkWidget *scrnSvrEBox = gtk_event_box_new();
  btnWid = gtk_label_new("L");
  gtk_container_add(GTK_CONTAINER(scrnSvrEBox), btnWid);
  gtk_widget_set_margin_start(scrnSvrEBox, 10);
  gtk_widget_set_margin_end(scrnSvrEBox, 10);

  g_signal_connect(scrnSvrEBox, "button-press-event",
                   G_CALLBACK(switchScreenSaverCb), this);

  gtk_grid_attach(GTK_GRID(grid), scrnSvrEBox, 8, 0, 1, 1);
}

void ScreenSaverModule::switchScreenSaverCb(GtkWidget *widget, GdkEvent *e,
                                            gpointer user_data) {
  ScreenSaverModule *self = static_cast<ScreenSaverModule *>(user_data);

  if (self->screenSaverMgr->inhibitCookie == -1) {
    if (!self->screenSaverMgr->activateScreenSaver()) {
      self->logger->LogInfo(TAG, "Screen Saver Activated.");
      gtk_label_set_label(GTK_LABEL(self->btnWid), "U");
    }
  } else {
    if (!self->screenSaverMgr->deactivateScreenSaver()) {
      self->logger->LogInfo(TAG, "Screen Saver Deactivated.");
      gtk_label_set_label(GTK_LABEL(self->btnWid), "L");
    }
  }
}
