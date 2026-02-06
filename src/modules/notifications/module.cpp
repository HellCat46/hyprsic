#include "module.hpp"
#include "../../utils/helper_func.hpp"
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
                                       NotificationManager *notifInstance)
    : notifInstance(notifInstance), ctx(ctx), dbManager(&ctx->dbManager) {}

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

  GtkWidget *mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_container_add(GTK_CONTAINER(menuWin), mainBox);
  gtk_widget_set_margin_start(mainBox, 10);
  gtk_widget_set_margin_end(mainBox, 10);
  gtk_widget_set_margin_top(mainBox, 10);
  gtk_widget_set_margin_bottom(mainBox, 10);

  // TopBar Box
  GtkWidget *topBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(mainBox), topBar, FALSE, FALSE, 0);

  GtkWidget *notifTitle = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(notifTitle),
                       "<big><b>Notifications</b></big>");
  gtk_box_pack_start(GTK_BOX(topBar), notifTitle, FALSE, FALSE, 0);

  GtkWidget *clearBtn = gtk_button_new_with_label("Clear All");
  gtk_box_pack_end(GTK_BOX(topBar), clearBtn, FALSE, FALSE, 0);
  g_signal_connect(clearBtn, "clicked", G_CALLBACK(handleClearAll), this);

  // Do Not Disturb Toggle
  GtkWidget *dndBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_widget_set_margin_bottom(dndBox, 10);
  gtk_widget_set_margin_start(dndBox, 10);
  gtk_widget_set_margin_end(dndBox, 10);

  GtkWidget *dndLbl = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(dndLbl), "<b>Do Not Disturb</b>");
  gtk_widget_set_halign(dndLbl, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(dndBox), dndLbl, FALSE, FALSE, 0);

  GtkWidget *dndSwitch = gtk_switch_new();
  gtk_box_pack_end(GTK_BOX(dndBox), dndSwitch, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(mainBox), dndBox, FALSE, FALSE, 0);
  g_signal_connect(dndSwitch, "state-set", G_CALLBACK(handleDndToggle), this);

  // Scrollable Window for Notifications
  GtkWidget *scrollWin = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(scrollWin, 400, 300);

  gtk_box_pack_start(GTK_BOX(mainBox), scrollWin, TRUE, TRUE, 0);

  scrollWinBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(scrollWin), scrollWinBox);

  gtk_widget_show_all(mainBox);
  update(true);
}

void NotificationModule::update(bool force) {
  if (!gtk_widget_get_visible(menuWin) && !force)
    return;

  for (auto notif = dbManager->notifList.begin();
       notif != dbManager->notifList.end(); ++notif) {
    // If we've already processed this notification, skip the whole list as they
    // are ordered by timestamp
    if (notifLookup.find(notif->id) != notifLookup.end())
      break;

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
                         ("<b>" + notif->app_name + "</b> - ").c_str());
    gtk_widget_set_halign(appName, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(topContent), appName, FALSE, FALSE, 0);

    GtkWidget *timestampLbl = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(timestampLbl),
                         ("<i>" + notif->timestamp + "</i>").c_str());
    gtk_widget_set_halign(timestampLbl, GTK_ALIGN_END);
    gtk_box_pack_start(GTK_BOX(topContent), timestampLbl, FALSE, FALSE, 0);

    GtkWidget *titleLbl = gtk_label_new(nullptr);
    gtk_label_set_markup(
        GTK_LABEL(titleLbl),
        (std::string("<b>Summary:</b> ") +
         (notif->summary.size() > 25
              ? HelperFunc::ValidString(notif->summary.substr(0, 22) + "...")
              : HelperFunc::ValidString(notif->summary)))
            .c_str());
    gtk_label_set_line_wrap(GTK_LABEL(titleLbl), TRUE);
    gtk_widget_set_halign(titleLbl, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(contentBox), titleLbl, FALSE, FALSE, 0);

    GtkWidget *bodyLbl = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(bodyLbl), (notif->body.c_str()));
    gtk_label_set_line_wrap(GTK_LABEL(bodyLbl), TRUE);
    gtk_widget_set_halign(bodyLbl, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(contentBox), bodyLbl, FALSE, FALSE, 0);

    GtkWidget *removeBtn = gtk_button_new_with_label("✖");
    gtk_box_pack_end(GTK_BOX(notifBox), removeBtn, FALSE, FALSE, 0);
    NotifFuncArgs *close_args = g_new0(NotifFuncArgs, 1);
    close_args->notifId = g_strdup(notif->id.c_str());
    close_args->dbManager = dbManager;
    close_args->notifLookup = &notifLookup;
    g_signal_connect_data(removeBtn, "clicked",
                          G_CALLBACK(NotificationModule::deleteNotificationCb),
                          close_args, (GClosureNotify)g_free, (GConnectFlags)0);

    gtk_container_add(GTK_CONTAINER(scrollWinBox), notifBox);
    gtk_widget_show_all(notifBox);

    notifLookup.insert({notif->id, {notif, notifBox}});
  }
}

void NotificationModule::showNotification(NotifFuncArgs *args) {

  NotifFuncArgs *close_args = g_new0(NotifFuncArgs, 1);
  close_args->notifId = g_strdup(args->notif->id.c_str());
  close_args->notifications = args->notifications;
  close_args->logger = args->logger;
  close_args->dbManager = args->dbManager;
  close_args->dnd = args->dnd;

  close_args->notif = new Notification();
  close_args->notif->app_name = args->notif->app_name.c_str();
  close_args->notif->summary = args->notif->summary.c_str();
  close_args->notif->body = args->notif->body.c_str();

  if (args->dnd) {
    autoCloseNotificationCb(close_args);
    return;
  }

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

  GtkWidget *mainBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_widget_set_margin_top(mainBox, 5);
  gtk_widget_set_margin_bottom(mainBox, 5);
  gtk_widget_set_margin_start(mainBox, 5);
  gtk_widget_set_margin_end(mainBox, 5);
  gtk_container_add(GTK_CONTAINER(window), mainBox);

  // Set Expire Timeout
  args->notif->expire_timeout = 5000;
  g_timeout_add_once(
      args->notif->expire_timeout,
      (GSourceOnceFunc)NotificationModule::autoCloseNotificationCb, close_args);

  // Logo of Application
  GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(
      args->notif->icon_pixbuf, 64, 64, GDK_INTERP_BILINEAR);
  GtkWidget *logo_box = gtk_image_new_from_pixbuf(scaled_pixbuf);
  g_object_ref(args->notif->icon_pixbuf);
  gtk_widget_set_size_request(logo_box, 5, 5);
  gtk_box_pack_start(GTK_BOX(mainBox), logo_box, FALSE, FALSE, 5);

  // Action Buttons - Close
  GtkWidget *action_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_end(GTK_BOX(mainBox), action_box, TRUE, TRUE, 5);
  GtkWidget *close_btn = gtk_button_new_with_label("✖");
  gtk_box_pack_start(GTK_BOX(action_box), close_btn, FALSE, FALSE, 0);
  g_signal_connect(close_btn, "clicked",
                   G_CALLBACK(NotificationModule::closeNotificationCb),
                   close_args);

  // Notification Texts
  GtkWidget *text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start(GTK_BOX(mainBox), text_box, TRUE, TRUE, 5);

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
  gtk_label_set_markup(GTK_LABEL(body_box),
                       HelperFunc::ValidString(args->notif->body));
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
    gtk_widget_destroy(it->second);
    args->notifications->erase(it->first);
  }
}

void NotificationModule::autoCloseNotificationCb(gpointer user_data) {
  NotifFuncArgs *args = static_cast<NotifFuncArgs *>(user_data);

  args->logger->LogInfo(
      TAG, "Auto-closing/Clearing Resources notification ID: " + std::string(args->notifId));

  // Save to DB that notification was closed due to timeout
  auto it = args->notifications->find(args->notifId);
  if (it != args->notifications->end() || args->dnd) {
    NotificationRecord record;
    record.id = args->notifId;
    record.app_name = args->notif->app_name;
    record.summary = args->notif->summary;
    record.body = args->notif->body;
    record.timestamp = args->dbManager->getCurrentTimestamp();

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

  auto it = args->notifLookup->find(args->notifId);
  if (it != args->notifLookup->end()) {

    args->dbManager->removeNotification(args->notifId, it->second.it);
    gtk_widget_destroy(it->second.widget);
    args->notifLookup->erase(it);
  }

  // // Free the arguments
  // g_free(args->notifId);
  // g_free(args);
}

void NotificationModule::handleDndToggle(GtkSwitch *widget, gboolean state,
                                         gpointer user_data) {
  NotificationModule *self = static_cast<NotificationModule *>(user_data);
  self->notifInstance->dnd = state;

  std::string msg =
      "Do Not Disturb Mode " + std::string(state ? "Enabled" : "Disabled");
  self->ctx->logger.LogInfo(TAG, msg);
  self->ctx->showUpdateWindow(UpdateModule::NOTIFICATIONS,
                              state ? "dnd_on" : "dnd_off", msg);
}

void NotificationModule::handleClearAll(GtkWidget *widget, gpointer user_data) {
  NotificationModule *self = static_cast<NotificationModule *>(user_data);
  self->dbManager->clearAllNotifications();

  for (auto &pair : self->notifLookup) {
    gtk_widget_destroy(pair.second.widget);
  }
  self->notifLookup.clear();
}
