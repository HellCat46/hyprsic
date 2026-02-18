#include "module.hpp"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "manager.hpp"
#include <map>
#include <string>

#define TAG "StatusNotifierModule"

StatusNotifierModule::StatusNotifierModule(
    AppContext *ctx, StatusNotifierManager *snManagerInstance) {
  snManager = snManagerInstance;
  logger = &ctx->logger;
}

GtkWidget* StatusNotifierModule::setup() {
  sniBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);

  snManager->removeCallbacks.push_back(
      {StatusNotifierModule::remove, sniBox, &sniApps});
  
  return sniBox;
}

void StatusNotifierModule::update() {
  for (const auto &[servicePath, appInfo] : snManager->registeredItems) {
    if (sniApps.find(servicePath) != sniApps.end())
      return; // Already Added

    GdkPixbuf *scaledPixbuf =
        gdk_pixbuf_scale_simple(appInfo.pixmap, 24, 24, GDK_INTERP_BILINEAR);
    SNIApp app;

    app.icon = gtk_event_box_new();
    GtkWidget *iconImg = gtk_image_new_from_pixbuf(scaledPixbuf);
    gtk_container_add(GTK_CONTAINER(app.icon), iconImg);

    app.popOver = gtk_popover_new(app.icon);
    gtk_widget_set_size_request(app.popOver, 200, -1);
    gtk_widget_set_margin_bottom(app.popOver, 30);

    MenuActionArgs *args = g_new0(MenuActionArgs, 1);
    args->snManager = snManager;
    args->itemId = servicePath;
    args->logger = logger;
    args->sniApp = app;

    // g_signal_connect(app.popOver, "focus-out-event",
    // G_CALLBACK(StatusNotifierModule::handleContextMenuOpen), args);
    g_signal_connect(app.icon, "button-press-event",
                     G_CALLBACK(StatusNotifierModule::handleContextMenuOpen),
                     args);

    gtk_box_pack_start(GTK_BOX(sniBox), app.icon, FALSE, FALSE, 0);
    gtk_widget_show_all(sniBox);

    sniApps.insert({servicePath, app});
  }
}

void StatusNotifierModule::remove(std::string servicePath,
                                  std::map<std::string, SNIApp> *sniApps,
                                  GtkWidget *sniBox) {
  auto it = sniApps->find(servicePath);
  if (it != sniApps->end()) {
    gtk_container_remove(GTK_CONTAINER(sniBox), it->second.icon);
    gtk_widget_destroy(it->second.icon);
    sniApps->erase(it);
  }
}

void StatusNotifierModule::handleContextMenuOpen(GtkWidget *widget,
                                                 GdkEventButton *event,
                                                 gpointer user_data) {

  MenuActionArgs *args = (MenuActionArgs *)user_data;
  if (gtk_widget_get_visible(args->sniApp.popOver)) {
    gtk_popover_popdown(GTK_POPOVER(args->sniApp.popOver));
    gtk_widget_destroy(args->sniApp.parentBox);
    return;
  }

  if (event->type != GDK_BUTTON_PRESS || event->button != 3)
    return; // Skip All but Right Clicks

  auto item = args->snManager->registeredItems.find(args->itemId);
  if (item == args->snManager->registeredItems.end())
    return;

  args->sniApp.parentBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_widget_set_margin_bottom(args->sniApp.parentBox, 10);
  gtk_widget_set_margin_top(args->sniApp.parentBox, 10);

  gtk_container_add(GTK_CONTAINER(args->sniApp.popOver),
                    args->sniApp.parentBox);

  for (const auto &[index, menuItem] : item->second.menuActions) {

    if (menuItem.isSeparator) {
      GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
      gtk_box_pack_start(GTK_BOX(args->sniApp.parentBox), separator, FALSE,
                         FALSE, 0);
    } else if (menuItem.visible) {
      GtkWidget *menuEvtBox = gtk_event_box_new();
      GtkWidget *menuBtn = gtk_label_new(menuItem.label.c_str());

      gtk_container_add(GTK_CONTAINER(menuEvtBox), menuBtn);
      gtk_widget_set_sensitive(menuBtn, menuItem.enabled);

      gtk_box_pack_start(GTK_BOX(args->sniApp.parentBox), menuEvtBox, FALSE,
                         FALSE, 0);
      
        EvtBtnPressArgs *btnArgs = g_new0(EvtBtnPressArgs, 1);
        btnArgs->snManager = args->snManager;
        btnArgs->itemId = args->itemId;
        btnArgs->logger = args->logger;
        btnArgs->sniApp = args->sniApp;
        btnArgs->evtIdx = index;
        g_signal_connect_data(
            menuEvtBox, "button-press-event",
            G_CALLBACK(StatusNotifierModule::handleEvtButtonPress), btnArgs,
            (GClosureNotify)g_free, (GConnectFlags)0);
    }
  }
  gtk_widget_show_all(args->sniApp.parentBox);

  gtk_popover_popup(GTK_POPOVER(args->sniApp.popOver));
}

void StatusNotifierModule::handleEvtButtonPress(GtkWidget *widget,
                                                 GdkEventButton *event,
                                                 gpointer user_data) {
  if (event->type != GDK_BUTTON_PRESS || event->button != 1)
    return;
  
  
  EvtBtnPressArgs *args = (EvtBtnPressArgs *)user_data;
  auto item = args->snManager->registeredItems.find(args->itemId);
  if (item == args->snManager->registeredItems.end())
    return;

  args->snManager->executeMenuAction(args->itemId, item->second.menu_path,
      event->time, args->evtIdx);
}
