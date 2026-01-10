#include "module.hpp"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "glib-object.h"
#include "glib.h"
#include "glibconfig.h"
#include "gtk-layer-shell.h"
#include "gtk/gtk.h"
#include "manager.hpp"
#include <cstring>

#define TAG "NotificationModule"

NotificationModule::NotificationModule(AppContext *ctx) : notifInstance(ctx) {
  logger = &ctx->logging;
  notifInstance.RunService(NotificationModule::showNotification,
                           &notifications);
}

void NotificationModule::setup(GtkWidget *box) {}
void NotificationModule::updateBox() {}

void NotificationModule::showNotification(NotifFuncArgs *args) {

  NotifFuncArgs *close_args = g_new0(NotifFuncArgs, 1);
  close_args->notifId = g_strdup(args->notif->id.c_str());
  close_args->notifications = args->notifications;
  close_args->logger = args->logger;
  close_args->dbManager = args->dbManager;

  close_args->notif = new Notification();
  close_args->notif->app_name = args->notif->app_name.c_str();
  close_args->notif->summary = args->notif->summary.c_str();
  close_args->notif->body = args->notif->body.c_str();

  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_layer_init_for_window(GTK_WINDOW(window));
  gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);

  gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);

  gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, 10);
  gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, 10);

  gtk_layer_set_exclusive_zone(GTK_WINDOW(window), 0);

  gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
  gtk_widget_set_size_request(window, 400, -1);

  GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_widget_set_margin_top(main_box, 5);
  gtk_widget_set_margin_bottom(main_box, 5);
  gtk_widget_set_margin_start(main_box, 5);
  gtk_widget_set_margin_end(main_box, 5);
  gtk_container_add(GTK_CONTAINER(window), main_box);

  // Set Expire Timeout
  args->notif->expire_timeout =
      args->notif->expire_timeout == 0 ? 5000 : args->notif->expire_timeout;
  g_timeout_add(args->notif->expire_timeout,
                (GSourceFunc)NotificationModule::autoCloseNotificationCb,
                close_args);

  // Logo of Application
  GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(
      args->notif->icon_pixbuf, 64, 64, GDK_INTERP_BILINEAR);
  GtkWidget *logo_box = gtk_image_new_from_pixbuf(scaled_pixbuf);
  g_object_ref(args->notif->icon_pixbuf);
  gtk_widget_set_size_request(logo_box, 5, 5);
  gtk_box_pack_start(GTK_BOX(main_box), logo_box, FALSE, FALSE, 5);

  // Action Buttons - Close
  GtkWidget *action_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_end(GTK_BOX(main_box), action_box, TRUE, TRUE, 5);
  GtkWidget *close_btn = gtk_button_new_with_label("✖");
  gtk_box_pack_start(GTK_BOX(action_box), close_btn, FALSE, FALSE, 0);
  g_signal_connect(close_btn, "clicked",
                   G_CALLBACK(NotificationModule::closeNotificationCb),
                   close_args);

  // Notification Texts
  GtkWidget *text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start(GTK_BOX(main_box), text_box, TRUE, TRUE, 5);

  GtkWidget *title_box = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(title_box),
                       ("<b>" + args->notif->summary + "</b>").c_str());
  gtk_label_set_line_wrap(GTK_LABEL(title_box), TRUE);
  gtk_widget_set_halign(title_box, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(text_box), title_box, FALSE, FALSE, 0);

  // Body Text
  if (args->notif->body.size() > 500) {
    args->notif->body = args->notif->body.substr(0, 497) + "...";
  }
  GtkWidget *body_box = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(body_box), args->notif->body.c_str());
  gtk_label_set_line_wrap(GTK_LABEL(body_box), TRUE);
  gtk_widget_set_halign(body_box, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(text_box), body_box, FALSE, FALSE, 0);

  gtk_widget_show_all(window);

  (*args->notifications).insert({args->notif->id, window});
}

void NotificationModule::closeNotificationCb(GtkWidget *widget,
                                             gpointer user_data) {

  NotifFuncArgs *args = static_cast<NotifFuncArgs *>(user_data);

  auto it = args->notifications->find(args->notifId);
  if (it != args->notifications->end()) {

    g_idle_add(
        [](gpointer data) -> gboolean {
          GtkWidget *win = static_cast<GtkWidget *>(data);
          gtk_widget_destroy(win);
          return G_SOURCE_REMOVE;
        },
        it->second);
    args->notifications->erase(it->first);
  }
}

gboolean NotificationModule::autoCloseNotificationCb(gpointer user_data) {
  NotifFuncArgs *args = static_cast<NotifFuncArgs *>(user_data);

  // Save to DB that notification was closed due to timeout
  auto it = args->notifications->find(args->notifId);
  if (it != args->notifications->end()) {
    args->logger->LogInfo(TAG, "Auto-closing notification ID: " +
                                   std::string(args->notifId));
    NotificationRecord record;
    record.id = args->notifId;
    record.app_name = args->notif->app_name;
    record.summary = args->notif->summary;
    record.body = args->notif->body;

    if (!args->dbManager->insertNotification(&record)) {
      args->logger->LogInfo(
          TAG, "Saved notification ID: " + std::string(args->notifId) +
                   " to database before auto-closing.");
    } else {
      args->logger->LogError(
          TAG, "Failed to save notification ID: " + std::string(args->notifId) +
                   " to database before auto-closing.");
    }
  }

  NotificationModule::closeNotificationCb(nullptr, user_data);
  g_free(args->notifId);
  delete args->notif;
  g_free(args);

  return G_SOURCE_REMOVE;
}
