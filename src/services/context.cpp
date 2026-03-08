#include "services/header/context.hpp"
#include "../utils/helper_func.hpp"
#include "cstring"
#include "dbus/dbus.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk-layer-shell.h"
#include "gtk/gtk.h"
#include "iostream"
#include "services/header/database.hpp"

#define TAG "AppContext"

AppContext::AppContext()
    : updateTimeoutId(0), dbus(), dbManager(&logger), logger(true) {}

DbusSystem::DbusSystem() : sysConn(nullptr), ssnConn(nullptr) {
  dbus_error_init(&sysErr);
  dbus_error_init(&ssnErr);
  sysConn = dbus_bus_get(DBUS_BUS_SYSTEM, &sysErr);
  if (!sysConn) {
    std::cerr << "[Error] Failed to Connect With the DBUS System: "
              << sysErr.name
              << "\n[DBUS Error Message - Context] : " << sysErr.message
              << std::endl;
    return;
  }

  ssnConn = dbus_bus_get(DBUS_BUS_SESSION, &ssnErr);
  if (!ssnConn) {
    std::cerr << "[Error] Failed to Connect With the DBUS Session: "
              << ssnErr.name
              << "\n[DBUS Error Message - Context] : " << ssnErr.message
              << std::endl;
  }

  dbus_threads_init_default();
}

DbusSystem::~DbusSystem() {
  dbus_error_free(&ssnErr);
  dbus_error_free(&sysErr);

  if (sysConn != nullptr) {
    dbus_connection_flush(sysConn);
    dbus_connection_close(sysConn);
  }

  if (ssnConn != nullptr) {
    dbus_connection_flush(ssnConn);
    dbus_connection_close(ssnConn);
  }
}

void DbusSystem::DictToInt64(DBusMessageIter *iter, uint64_t &outValue) {
  DBusMessageIter variantIter;
  dbus_message_iter_recurse(iter, &variantIter);

  if (dbus_message_iter_get_arg_type(&variantIter) == DBUS_TYPE_INT64) {
    dbus_message_iter_get_basic(&variantIter, &outValue);
  }
}

void DbusSystem::DictToString(DBusMessageIter *iter, std::string &outValue) {
  DBusMessageIter variantIter;
  dbus_message_iter_recurse(iter, &variantIter);

  if (dbus_message_iter_get_arg_type(&variantIter) == DBUS_TYPE_STRING ||
      dbus_message_iter_get_arg_type(&variantIter) == DBUS_TYPE_OBJECT_PATH) {
    char *strValue;
    dbus_message_iter_get_basic(&variantIter, &strValue);
    outValue = strValue;
  }
}

void AppContext::initWindows() {
  setupUpdateWindow();
  setupNotifWindow();
  setupCtrlWindow();
}

void AppContext::setupUpdateWindow() {
  updateTimeoutId = 0;
  updateWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_layer_init_for_window(GTK_WINDOW(updateWin));
  gtk_layer_set_layer(GTK_WINDOW(updateWin), GTK_LAYER_SHELL_LAYER_OVERLAY);

  gtk_layer_set_anchor(GTK_WINDOW(updateWin), GTK_LAYER_SHELL_EDGE_TOP, false);
  gtk_layer_set_anchor(GTK_WINDOW(updateWin), GTK_LAYER_SHELL_EDGE_BOTTOM,
                       false);
  gtk_layer_set_anchor(GTK_WINDOW(updateWin), GTK_LAYER_SHELL_EDGE_LEFT, false);
  gtk_layer_set_anchor(GTK_WINDOW(updateWin), GTK_LAYER_SHELL_EDGE_RIGHT,
                       false);

  gtk_layer_set_exclusive_zone(GTK_WINDOW(updateWin), 0);
  gtk_widget_set_opacity(updateWin, 0.95);

  updateWinGrid = GTK_GRID(gtk_grid_new());
  gtk_widget_set_margin_top(GTK_WIDGET(updateWinGrid), 20);
  gtk_widget_set_margin_bottom(GTK_WIDGET(updateWinGrid), 20);
  gtk_widget_set_margin_start(GTK_WIDGET(updateWinGrid), 20);
  gtk_widget_set_margin_end(GTK_WIDGET(updateWinGrid), 20);
  gtk_container_add(GTK_CONTAINER(updateWin), GTK_WIDGET(updateWinGrid));

  updateIcon = gtk_image_new();
  gtk_grid_attach(updateWinGrid, updateIcon, 0, 0, 1, 4);

  updateMsg = gtk_label_new("");
  gtk_widget_set_margin_top(updateMsg, 10);
  gtk_label_set_line_wrap(GTK_LABEL(updateMsg), true);
  gtk_widget_set_size_request(updateMsg, 200, -1);

  gtk_grid_attach(updateWinGrid, updateMsg, 0, 4, 1, 1);
}

void AppContext::setupNotifWindow() {
  closeNotifId = 0;
  notifWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_layer_init_for_window(GTK_WINDOW(notifWin));
  gtk_layer_set_layer(GTK_WINDOW(notifWin), GTK_LAYER_SHELL_LAYER_OVERLAY);

  gtk_layer_set_anchor(GTK_WINDOW(notifWin), GTK_LAYER_SHELL_EDGE_TOP, true);
  gtk_layer_set_anchor(GTK_WINDOW(notifWin), GTK_LAYER_SHELL_EDGE_RIGHT, true);

  gtk_layer_set_margin(GTK_WINDOW(notifWin), GTK_LAYER_SHELL_EDGE_TOP, 10);
  gtk_layer_set_margin(GTK_WINDOW(notifWin), GTK_LAYER_SHELL_EDGE_RIGHT, 10);

  gtk_layer_set_exclusive_zone(GTK_WINDOW(notifWin), 0);

  gtk_window_set_decorated(GTK_WINDOW(notifWin), false);
  gtk_widget_set_size_request(notifWin, 400, -1);

  notifEvtBox = gtk_event_box_new();
  GtkWidget *notifBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_widget_set_margin_top(notifBox, 5);
  gtk_widget_set_margin_bottom(notifBox, 5);
  gtk_widget_set_margin_start(notifBox, 5);
  gtk_widget_set_margin_end(notifBox, 5);
  gtk_container_add(GTK_CONTAINER(notifEvtBox), notifBox);
  gtk_container_add(GTK_CONTAINER(notifWin), notifEvtBox);

  notifLogo = gtk_image_new();
  gtk_widget_set_size_request(notifLogo, 64, 64);
  gtk_box_pack_start(GTK_BOX(notifBox), notifLogo, false, false, 5);

  GtkWidget *notifTextBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start(GTK_BOX(notifBox), notifTextBox, true, true, 5);

  notifTitle = gtk_label_new(nullptr);
  gtk_label_set_line_wrap(GTK_LABEL(notifTitle), true);
  gtk_widget_set_halign(notifTitle, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(notifTextBox), notifTitle, false, false, 0);

  notifBody = gtk_label_new(nullptr);
  gtk_label_set_line_wrap(GTK_LABEL(notifBody), true);
  gtk_widget_set_halign(notifBody, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(notifTextBox), notifBody, false, false, 0);
}

void AppContext::setupCtrlWindow() {
  ctrlWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_layer_init_for_window(GTK_WINDOW(ctrlWin));
  gtk_layer_set_keyboard_mode(GTK_WINDOW(ctrlWin),
                              GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);

  GtkEventController *evtCtrl = gtk_event_controller_key_new(ctrlWin);
  g_signal_connect(evtCtrl, "key-released",
                   G_CALLBACK(AppContext::handleKeyPress), ctrlWin);

  GdkScreen *screen = gtk_widget_get_screen(ctrlWin);
  GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
  if (visual != nullptr && gdk_screen_is_composited(screen)) {
    gtk_widget_set_visual(ctrlWin, visual);
  }

  gtk_layer_set_anchor(GTK_WINDOW(ctrlWin), GTK_LAYER_SHELL_EDGE_TOP, false);
  gtk_layer_set_anchor(GTK_WINDOW(ctrlWin), GTK_LAYER_SHELL_EDGE_BOTTOM, false);
  gtk_layer_set_anchor(GTK_WINDOW(ctrlWin), GTK_LAYER_SHELL_EDGE_LEFT, false);
  gtk_layer_set_anchor(GTK_WINDOW(ctrlWin), GTK_LAYER_SHELL_EDGE_RIGHT, false);

  gtk_widget_set_margin_start(ctrlWin, 50);
  gtk_widget_set_margin_end(ctrlWin, 50);
  gtk_widget_set_margin_top(ctrlWin, 30);
  gtk_widget_set_margin_bottom(ctrlWin, 30);

  GtkWidget *mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

  moduleStk = gtk_stack_new();
  gtk_stack_set_transition_type(GTK_STACK(moduleStk),
                                GTK_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_stack_set_homogeneous(GTK_STACK(moduleStk), false);

  gtk_box_pack_start(GTK_BOX(mainBox), moduleStk, true, true, 0);
  gtk_container_add(GTK_CONTAINER(ctrlWin), mainBox);

  gtk_widget_set_app_paintable(ctrlWin, true);
  const char *css = ".win { "
                    "  background-color: transparent; "
                    "}"
                    ".mainBox { "
                    "  background-color: @theme_bg_color; "
                    "  border: 2px solid @borders; "
                    "  border-radius: 12px; "
                    "  margin: 5px; "
                    "}";

  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(provider, css, -1, NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                            GTK_STYLE_PROVIDER(provider),
                                            GTK_STYLE_PROVIDER_PRIORITY_USER);

  GtkStyleContext *boxCtx = gtk_widget_get_style_context(mainBox);
  gtk_style_context_add_class(boxCtx, "mainBox");

  GtkStyleContext *winCtx = gtk_widget_get_style_context(ctrlWin);
  gtk_style_context_add_class(winCtx, "win");
}

void AppContext::showCtrlWindow(const std::string &moduleName, gint width,
                                gint height) {

  logger.LogInfo(TAG, "Showing Control Window for Module: " + moduleName);

  auto data = new CtrlWindowData{moduleName, width, height, this};
  g_idle_add_once(
      [](gpointer data) {
        CtrlWindowData *cwData = static_cast<CtrlWindowData *>(data);

        gtk_stack_set_visible_child_name(GTK_STACK(cwData->ctx->moduleStk),
                                         cwData->moduleName.c_str());

        cwData->ctx->logger.LogDebug(
            TAG, "Set Visible Child in Stack: " + cwData->moduleName +
                     " with Size: " + std::to_string(cwData->width) + "x" +
                     std::to_string(cwData->height));

        gtk_widget_set_size_request(cwData->ctx->ctrlWin, cwData->width,
                                    cwData->height);
        gtk_widget_show_all(cwData->ctx->ctrlWin);

        delete cwData;
      },
      data);
}

void AppContext::addModule(GtkWidget *moduleBox,
                           const std::string &moduleName) {
  gint width, height;
  gtk_widget_get_size_request(moduleBox, &width, &height);
  logger.LogDebug(TAG, "Adding Module to Stack: " + moduleName +
                           " with Size: " + std::to_string(width) + "x" +
                           std::to_string(height));

  gtk_stack_add_named(GTK_STACK(moduleStk), moduleBox, moduleName.c_str());
}

bool AppContext::showUpdateWindow(UpdateModule module, std::string type,
                                  std::string msg) {
  if (updateTimeoutId != 0) {
    g_source_remove(updateTimeoutId);
  }

  std::string iconPath;
  switch (module) {
  case UpdateModule::MPRIS:
    iconPath += "mpris_" + type;
    break;
  case UpdateModule::NOTIFICATIONS:
    iconPath += "notifications_" + type;
    break;
  case UpdateModule::BLUETOOTH:
    iconPath += "bluetooth_" + type;
    break;
  case UpdateModule::SCREENSAVER:
    iconPath += "screensaver_" + type;
    break;
  case UpdateModule::PULSEAUDIO:
    iconPath += "audio_" + type;
    break;
  case UpdateModule::WIFI:
    iconPath += "wifi_" + type;
    break;
  case UpdateModule::BATTERY:
    iconPath += "battery_" + type;
    break;
  case UpdateModule::SYSINFO:
    iconPath += "sysinfo_" + type;
    break;
  }

  auto icon = resStore.icons.find(iconPath);
  if (icon == resStore.icons.end()) {
    logger.LogError(TAG, "Icon Not Found for Update Window: " + iconPath);
    return 1;
  }

  GInputStream *stream = g_memory_input_stream_new_from_data(
      icon->second.data(), icon->second.size(), nullptr);
  if (stream == nullptr) {
    logger.LogError(TAG,
                    "Failed to Create GInputStream for Update Window Icon: " +
                        iconPath);
    return 1;
  }

  GError *error = nullptr;
  GdkPixbuf *pixBuf = gdk_pixbuf_new_from_stream(stream, nullptr, &error);
  if (!pixBuf) {
    logger.LogError(TAG, error->message);
    return 1;
  }

  g_object_unref(stream);

  // Update the UI in the Main Thread
  auto data = new UpdateWindowData{module, type, msg, this, pixBuf};
  g_idle_add_once(
      [](gpointer data) {
        UpdateWindowData *updateData = static_cast<UpdateWindowData *>(data);
        AppContext *ctx = updateData->ctx;

        gtk_image_set_from_pixbuf(GTK_IMAGE(ctx->updateIcon),
                                  updateData->pixBuf);
        g_object_unref(updateData->pixBuf);

        gtk_label_set_markup(GTK_LABEL(ctx->updateMsg),
                             ("<b>" + updateData->msg + "</b>").c_str());
        gtk_widget_show_all(ctx->updateWin);

        delete updateData;
      },
      data);

  updateTimeoutId = g_timeout_add_once(2000, hideUpdateWindow, this);
  return 0;
}

void AppContext::showNotifWindow(Notification *notif, bool dnd) {
  auto winData =
      new NotifWindowData{NotificationRecord{
                              .id = notif->id,
                              .app_name = notif->app_name,
                              .summary = notif->summary,
                              .body = notif->body,
                              .timestamp = dbManager.getCurrentTimestamp(),
                          },
                          this, dnd};
  if (dnd) {
    autoCloseNotificationCb(winData);
    return;
  }

  g_timeout_add_once(5000, autoCloseNotificationCb, winData);

  // Signal Connection Handling
  if (closeNotifId > 0)
    g_signal_handler_disconnect(notifEvtBox, closeNotifId);

  closeNotifId = g_signal_connect_data(notifEvtBox, "button-press-event",
                                       G_CALLBACK(closeNotificationCb), this,
                                       nullptr, (GConnectFlags)0);

  // Adding Data to UI Elements
  gtk_label_set_markup(GTK_LABEL(notifTitle),
                       ("<b>" + notif->summary + "</b>").c_str());

  if (notif->body.size() > 500) {
    notif->body = notif->body.substr(0, 497) + "...";
  }
  gtk_label_set_markup(GTK_LABEL(notifBody),
                       HelperFunc::ValidString(notif->body));

  if (notif->icon_pixbuf) {
    gtk_image_set_from_pixbuf(GTK_IMAGE(notifLogo), notif->icon_pixbuf);
    g_object_unref(notif->icon_pixbuf);
  } else {
    gtk_image_clear(GTK_IMAGE(notifLogo));
  }

  gtk_widget_show_all(notifWin);
}

void AppContext::closeNotificationCb([[maybe_unused]] GtkWidget *widget,
                                     [[maybe_unused]] GdkEvent *e,
                                     gpointer user_data) {
  AppContext *self = static_cast<AppContext *>(user_data);
  gtk_widget_hide(self->notifWin);
}

void AppContext::autoCloseNotificationCb(gpointer user_data) {
  NotifWindowData *args = static_cast<NotifWindowData *>(user_data);

  // Handles the case where notification is already closed by user interaction
  // before timeout
  if (!gtk_widget_is_visible(args->ctx->notifWin) && !args->dnd) {
    delete args;
    return;
  }

  closeNotificationCb(nullptr, nullptr, args->ctx);

  // Save to DB that notification was closed due to timeout
  if (!args->ctx->dbManager.insertNotification(&args->notif)) {
    args->ctx->logger.LogInfo(TAG, "Saved notification ID: " + args->notif.id +
                                       " to database before auto-closing.");
  } else {
    args->ctx->logger.LogError(
        TAG, "Failed to save notification ID: " + args->notif.id +
                 " to database before auto-closing.");
  }

  delete args;
}

void AppContext::hideUpdateWindow(gpointer user_data) {
  AppContext *self = static_cast<AppContext *>(user_data);
  gtk_widget_hide(self->updateWin);

  self->updateTimeoutId = 0;
}

gboolean AppContext::handleKeyPress([[maybe_unused]] GtkWidget *wid,
                                    guint keyval,
                                    [[maybe_unused]] guint keycode,
                                    [[maybe_unused]] GdkModifierType state,
                                    gpointer data) {

  GtkWidget *window = static_cast<GtkWidget *>(data);

  if (keyval == GDK_KEY_Escape) {
    gtk_widget_hide(window);
    return true;
  }
  return false;
}
