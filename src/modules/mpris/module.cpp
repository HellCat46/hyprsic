#include "module.hpp"
#include "../../utils/helper_func.hpp"
#include "manager.hpp"
#include "window.hpp"
#include <string>

#define TAG "MprisModule"

MprisModule::MprisModule(AppContext *ctx, MprisManager *mprisMgr,
                         MprisWindow *mprisWindow)
    : ctx(ctx), manager(mprisMgr), window(mprisWindow) {}

GtkWidget *MprisModule::setup() {
  mainLbl = gtk_label_new(nullptr);
  gtk_label_set_ellipsize(GTK_LABEL(mainLbl), PANGO_ELLIPSIZE_END);
  GtkWidget *labelAction = gtk_event_box_new();
  g_signal_connect(labelAction, "button-press-event",
                   G_CALLBACK(MprisModule::chgVisibilityMenu), this);

  gtk_container_add(GTK_CONTAINER(labelAction), mainLbl);
  gtk_widget_set_hexpand(labelAction, FALSE);

  update();
  return labelAction;
}

void MprisModule::update() {
  if (!manager->hasPlayer)
    return;

  gchar *finalText = HelperFunc::ValidString(manager->playingTrack.title);
  std::string title = "<span foreground='green'><b>";
  title += finalText;
  title += "</b></span>";

  g_free(finalText);
  gtk_label_set_markup(GTK_LABEL(mainLbl), title.c_str());
}

void MprisModule::chgVisibilityMenu(GtkWidget *widget, GdkEvent *e,
                                    gpointer user_data) {
  MprisModule *self = static_cast<MprisModule *>(user_data);

  if (self->window->isVisible()) {
    self->window->chgVisibility(false);
    return;
  }

  self->ctx->logger.LogInfo(TAG, "Clicked on MprisModule, button: " +
                                     std::to_string(e->button.button));

  if (e->button.button == 3) {

    self->update();
    self->window->chgVisibility(true);
  } else {
    self->manager->PlayPause();
  }
}
