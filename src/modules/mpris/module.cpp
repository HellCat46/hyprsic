#include "module.hpp"
#include "glibconfig.h"
#include "gtk/gtk.h"

#define TAG "MprisModule"

MprisModule::MprisModule(AppContext *ctx) : mprisInstance(ctx) {
  logger = &ctx->logging;
}

void MprisModule::setup(GtkWidget *mainBox) {
  GtkWidget *middleBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_pack_end(GTK_BOX(mainBox), middleBox, FALSE, FALSE, 0);

  mainLabel = gtk_label_new(nullptr);
  GtkWidget *labelAction = gtk_event_box_new();
    g_signal_connect(labelAction, "button-press-event",
                     G_CALLBACK(MprisModule::handlePlayPause), &mprisInstance);

  gtk_container_add(GTK_CONTAINER(labelAction), mainLabel);
  gtk_box_pack_start(GTK_BOX(middleBox), labelAction, FALSE, FALSE, 0);

  Update();

  gtk_widget_show_all(mainBox);
}


void MprisModule::Update() {
  if (!mprisInstance.GetTitle()) {
    gtk_label_set_markup(GTK_LABEL(mainLabel), ("<span foreground='green'><b>"+
                        mprisInstance.showPlayerTitle.second + "</b></span>").c_str());
  }
}

void MprisModule::handlePlayPause(GtkWidget *widget, GdkEvent *e,
                                 gpointer user_data) {
  MprisManager *mprisInstance = static_cast<MprisManager *>(user_data);
  mprisInstance->PlayPause();
}