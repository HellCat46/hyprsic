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
  btnWid = gtk_button_new_with_label("L");
  g_signal_connect(btnWid, "clicked", G_CALLBACK(switchScreenSaverCb), this);

  gtk_grid_attach(GTK_GRID(grid), btnWid, 8, 0, 1, 1);
}

void ScreenSaverModule::switchScreenSaverCb(GtkWidget *widget,
                                            gpointer user_data) {
  ScreenSaverModule *self = static_cast<ScreenSaverModule *>(user_data);

  if (self->screenSaverMgr->inhibitCookie == -1) {
    if (!self->screenSaverMgr->activateScreenSaver()) {
      self->logger->LogInfo(TAG, "Screen Saver Activated.");
      gtk_button_set_label(GTK_BUTTON(self->btnWid), "U");
    }
  } else {
    if (!self->screenSaverMgr->deactivateScreenSaver()) {
      self->logger->LogInfo(TAG, "Screen Saver Deactivated.");
      gtk_button_set_label(GTK_BUTTON(self->btnWid), "L");
    }
  }
}
