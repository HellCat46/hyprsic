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

NotificationModule::NotificationModule(AppContext *ctx,
                                       NotificationManager *notifInstance) {
  notifInstance = notifInstance;
  logger = &ctx->logging;
  dbManager = &ctx->dbManager;
}

void NotificationModule::setup(GtkWidget *box) {
  GtkWidget *notifEBox = gtk_event_box_new();
  GtkWidget *notif = gtk_label_new("");
  gtk_container_add(GTK_CONTAINER(notifEBox), notif);
  gtk_grid_attach(GTK_GRID(box), notifEBox, 7, 0, 1, 1);
  gtk_widget_set_margin_start(notifEBox, 10);

  g_signal_connect(notifEBox, "button-press-event",
                   G_CALLBACK(NotificationModule::chgVisibiltyWin), this);

  menuWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_layer_init_for_window(GTK_WINDOW(menuWin));
  gtk_layer_set_layer(GTK_WINDOW(menuWin), GTK_LAYER_SHELL_LAYER_OVERLAY);
  gtk_layer_set_anchor(GTK_WINDOW(menuWin), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(menuWin), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);

  GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_container_add(GTK_CONTAINER(menuWin), main_box);
  gtk_widget_set_margin_start(main_box, 10);
  gtk_widget_set_margin_end(main_box, 10);
  gtk_widget_set_margin_top(main_box, 10);
  gtk_widget_set_margin_bottom(main_box, 10);

  // Title Bar with Close Button

  GtkWidget *notifTitle = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(notifTitle), "<b>Notifications</b>");
  gtk_box_pack_start(GTK_BOX(main_box), notifTitle, FALSE, FALSE, 0);

  // Scrollable Window for Notifications
  GtkWidget *scrollWin = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(scrollWin, 400, 300);

  gtk_box_pack_start(GTK_BOX(main_box), scrollWin, TRUE, TRUE, 0);

  scrollWinBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(scrollWin), scrollWinBox);

  gtk_widget_show_all(main_box);
}

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

void NotificationModule::chgVisibiltyWin(GtkWidget *widget, GdkEvent *e,
                                         gpointer user_data) {
  NotificationModule *self = static_cast<NotificationModule *>(user_data);

  if (!gtk_widget_get_visible(self->menuWin)) {
    self->update();
    gtk_widget_show(self->menuWin);
  } else {
    gtk_widget_hide(self->menuWin);
  }
}

void NotificationModule::deleteNotificationCb(GtkWidget *widget,
                                              gpointer user_data) {
  NotifFuncArgs *args = static_cast<NotifFuncArgs *>(user_data);

  args->dbManager->removeNotification(args->notifId);
}

void NotificationModule::update() {
  if (!gtk_widget_get_visible(menuWin))
    return;

  GList *children = gtk_container_get_children(GTK_CONTAINER(scrollWinBox));
  for (GList *iter = children; iter != nullptr; iter = iter->next) {
    gtk_widget_destroy(GTK_WIDGET(iter->data));
  }
  g_list_free(children);

  for (const auto &[notifId, notif] : dbManager->notificationCache) {
    GtkWidget *notifBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_top(notifBox, 5);
    gtk_widget_set_margin_bottom(notifBox, 5);
    gtk_widget_set_margin_start(notifBox, 5);
    gtk_widget_set_margin_end(notifBox, 5);

    GtkWidget *contentBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(notifBox), contentBox, TRUE, TRUE, 0);

    GtkWidget *topContent = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(contentBox), topContent, FALSE, FALSE, 0);

    GtkWidget *appName = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(appName),
                         ("<b>" + notif.app_name + "</b> - ").c_str());
    gtk_widget_set_halign(appName, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(topContent), appName, FALSE, FALSE, 0);

    GtkWidget *timestampLbl = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(timestampLbl),
                         ("<i>" + notif.timestamp + "</i>").c_str());
    gtk_widget_set_halign(timestampLbl, GTK_ALIGN_END);
    gtk_box_pack_start(GTK_BOX(topContent), timestampLbl, FALSE, FALSE, 0);

    GtkWidget *titleLbl = gtk_label_new(nullptr);
    gtk_label_set_markup(
        GTK_LABEL(titleLbl),
        ("<b>Summary:</b> " + (notif.summary.size() > 25
                                   ? notif.summary.substr(0, 22) + "..."
                                   : notif.summary))
            .c_str());
    gtk_label_set_line_wrap(GTK_LABEL(titleLbl), TRUE);
    gtk_widget_set_halign(titleLbl, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(contentBox), titleLbl, FALSE, FALSE, 0);

    GtkWidget *bodyLbl = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(bodyLbl), notif.body.c_str());
    gtk_label_set_line_wrap(GTK_LABEL(bodyLbl), TRUE);
    gtk_widget_set_halign(bodyLbl, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(contentBox), bodyLbl, FALSE, FALSE, 0);

    GtkWidget *removeBtn = gtk_button_new_with_label("✖");
    gtk_box_pack_end(GTK_BOX(notifBox), removeBtn, FALSE, FALSE, 0);
    NotifFuncArgs *close_args = g_new0(NotifFuncArgs, 1);
    close_args->notifId = g_strdup(notifId.c_str());
    close_args->dbManager = dbManager;
    g_signal_connect_data(removeBtn, "clicked",
                          G_CALLBACK(NotificationModule::deleteNotificationCb),
                          close_args, (GClosureNotify)g_free, (GConnectFlags)0);

    gtk_container_add(GTK_CONTAINER(scrollWinBox), notifBox);

    gtk_widget_show_all(notifBox);
  }
}
