#include "context.hpp"
#include "cstring"
#include "dbus/dbus.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk-layer-shell.h"
#include "gtk/gtk.h"
#include "iostream"

#define TAG "AppContext"

AppContext::AppContext()
    : dbus(), logger(true), dbManager(&logger), updateTimeoutId(0) {}

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
  // Setting Up Update Window
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

  // Setting Up Control Window
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

  gtk_box_pack_start(GTK_BOX(mainBox), moduleStk, TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(ctrlWin), mainBox);

  gtk_widget_set_app_paintable(ctrlWin, TRUE);
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

void AppContext::hideUpdateWindow(gpointer user_data) {
  AppContext *self = static_cast<AppContext *>(user_data);
  gtk_widget_hide(self->updateWin);

  self->updateTimeoutId = 0;
}

gboolean AppContext::handleKeyPress(GtkWidget *wid, guint keyval, guint keycode,
                                    GdkModifierType state, gpointer data) {

  GtkWidget *window = static_cast<GtkWidget *>(data);

  if (keyval == GDK_KEY_Escape) {
    gtk_widget_hide(window);
    return true;
  }
  return false;
}
