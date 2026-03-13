#include "header/window.hpp"
#include "../../utils/helper_func.hpp"

#define TAG "NotificationWindow"

NotificationWindow::NotificationWindow(AppContext *ctx,
                                       NotificationManager *manager)
    : ctx(ctx), manager(manager) {}

void NotificationWindow::init() {
  menuBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(menuBox, 10);
  gtk_widget_set_margin_end(menuBox, 10);
  gtk_widget_set_margin_top(menuBox, 10);
  gtk_widget_set_margin_bottom(menuBox, 10);

  // TopBar Box
  GtkWidget *topBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(menuBox), topBar, false, false, 0);

  GtkWidget *notifTitle = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(notifTitle),
                       "<big><b>Notifications</b></big>");
  gtk_box_pack_start(GTK_BOX(topBar), notifTitle, false, false, 0);

  GtkWidget *clearBtn = gtk_button_new_with_label("Clear All");
  gtk_box_pack_end(GTK_BOX(topBar), clearBtn, false, false, 0);
  g_signal_connect(clearBtn, "clicked", G_CALLBACK(handleClearAll), this);

  // Do Not Disturb Toggle
  GtkWidget *dndBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_widget_set_margin_bottom(dndBox, 10);
  gtk_widget_set_margin_start(dndBox, 10);
  gtk_widget_set_margin_end(dndBox, 10);

  GtkWidget *dndLbl = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(dndLbl), "<b>Do Not Disturb</b>");
  gtk_widget_set_halign(dndLbl, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(dndBox), dndLbl, false, false, 0);

  GtkWidget *dndSwitch = gtk_switch_new();
  gtk_box_pack_end(GTK_BOX(dndBox), dndSwitch, false, false, 0);
  gtk_box_pack_start(GTK_BOX(menuBox), dndBox, false, false, 0);
  g_signal_connect(dndSwitch, "state-set", G_CALLBACK(handleDndToggle), this);

  // Scrollable Window for Notifications
  GtkWidget *scrollWin = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(scrollWin, 400, 300);

  gtk_box_pack_start(GTK_BOX(menuBox), scrollWin, true, true, 0);

  scrollWinBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(scrollWin), scrollWinBox);

  gtk_widget_show_all(menuBox);
  ctx->addModule(menuBox, "notifications");
  update(true);
}

void NotificationWindow::update(bool force) {
  if (!gtk_widget_get_visible(menuBox) && !force)
    return;

  for (auto notif = ctx->dbManager.notifList.begin();
       notif != ctx->dbManager.notifList.end(); ++notif) {
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
    gtk_box_pack_start(GTK_BOX(notifBox), contentBox, true, true, 0);

    GtkWidget *topContent = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(contentBox), topContent, false, false, 0);

    GtkWidget *appName = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(appName),
                         ("<b>" + notif->app_name + "</b> - ").c_str());
    gtk_widget_set_halign(appName, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(topContent), appName, false, false, 0);

    GtkWidget *timestampLbl = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(timestampLbl),
                         ("<i>" + notif->timestamp + "</i>").c_str());
    gtk_widget_set_halign(timestampLbl, GTK_ALIGN_END);
    gtk_box_pack_start(GTK_BOX(topContent), timestampLbl, false, false, 0);

    GtkWidget *titleLbl = gtk_label_new(nullptr);
    gtk_label_set_markup(
        GTK_LABEL(titleLbl),
        (std::string("<b>Summary:</b> ") +
         (notif->summary.size() > 25
              ? HelperFunc::ValidString(notif->summary.substr(0, 22) + "...")
              : HelperFunc::ValidString(notif->summary)))
            .c_str());
    gtk_label_set_line_wrap(GTK_LABEL(titleLbl), true);
    gtk_widget_set_halign(titleLbl, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(contentBox), titleLbl, false, false, 0);

    GtkWidget *bodyLbl = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(bodyLbl), (notif->body.c_str()));
    gtk_label_set_line_wrap(GTK_LABEL(bodyLbl), true);
    gtk_widget_set_halign(bodyLbl, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(contentBox), bodyLbl, false, false, 0);

    GtkWidget *removeBtn = gtk_button_new_with_label("✖");
    gtk_box_pack_end(GTK_BOX(notifBox), removeBtn, false, false, 0);
    NotifFuncArgs *close_args = g_new0(NotifFuncArgs, 1);
    close_args->notifId = g_strdup(notif->id.c_str());
    close_args->dbManager = &ctx->dbManager;
    close_args->notifLookup = &notifLookup;
    g_signal_connect_data(removeBtn, "clicked",
                          G_CALLBACK(NotificationWindow::deleteNotificationCb),
                          close_args, (GClosureNotify)g_free, (GConnectFlags)0);

    gtk_container_add(GTK_CONTAINER(scrollWinBox), notifBox);
    gtk_widget_show_all(notifBox);

    notifLookup.insert({notif->id, {notif, notifBox}});
  }
}

void NotificationWindow::deleteNotificationCb([[maybe_unused]] GtkWidget *widget,
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

void NotificationWindow::handleDndToggle([[maybe_unused]] GtkSwitch *widget, gboolean state,
                                         gpointer user_data) {
  NotificationWindow *self = static_cast<NotificationWindow *>(user_data);
  self->manager->dnd = state;

  std::string msg =
      "Do Not Disturb Mode " + std::string(state ? "Enabled" : "Disabled");
  self->ctx->logger.LogInfo(TAG, msg);
  self->ctx->showUpdateWindow(UpdateModule::NOTIFICATIONS,
                              state ? "dnd_on" : "dnd_off", msg);
}

void NotificationWindow::handleClearAll([[maybe_unused]] GtkWidget *widget, gpointer user_data) {
  NotificationWindow *self = static_cast<NotificationWindow *>(user_data);
  self->ctx->dbManager.clearAllNotifications();

  for (auto &pair : self->notifLookup) {
    gtk_widget_destroy(pair.second.widget);
  }
  self->notifLookup.clear();
}
