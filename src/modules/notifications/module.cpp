#include "module.hpp"
#include "manager.hpp"
#include <cstring>

#define TAG "NotificationModule"

NotificationModule::NotificationModule(AppContext *ctx,
                                       NotificationManager *notifInstance,
                                       NotificationWindow *window)
    : manager(notifInstance), window(window), ctx(ctx) {}

GtkWidget *NotificationModule::setup() {
  GtkWidget *notifEBox = gtk_event_box_new();
  GtkWidget *notif = gtk_label_new("");
  gtk_container_add(GTK_CONTAINER(notifEBox), notif);
  gtk_widget_set_margin_start(notifEBox, 10);

  g_signal_connect(notifEBox, "button-press-event",
                   G_CALLBACK(NotificationModule::chgVisibiltyWin), this);

  return notifEBox;
}

void NotificationModule::chgVisibiltyWin(GtkWidget *widget, GdkEvent *e,
                                         gpointer user_data) {
  NotificationModule *self = static_cast<NotificationModule *>(user_data);

  self->window->update();
  self->ctx->showCtrlWindow("notifications", 420, 400);
}
