#include "context.hpp"
#include "cstring"
#include "dbus/dbus.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "glib.h"
#include "gtk-layer-shell.h"
#include "gtk/gtk.h"
#include "iostream"

#define TAG "AppContext"

AppContext::AppContext() : dbus(), logger(true), dbManager(&logger) {}

DbusSystem::DbusSystem() {
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

  dbus_connection_flush(sysConn);
  dbus_connection_close(sysConn);

  dbus_connection_flush(ssnConn);
  dbus_connection_close(ssnConn);
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

void AppContext::initUpdateWindow() {
  // Setting Up Update Window
  updateWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_layer_init_for_window(GTK_WINDOW(updateWindow));
  gtk_layer_set_layer(GTK_WINDOW(updateWindow), GTK_LAYER_SHELL_LAYER_OVERLAY);

  gtk_layer_set_anchor(GTK_WINDOW(updateWindow), GTK_LAYER_SHELL_EDGE_TOP,
                       false);
  gtk_layer_set_anchor(GTK_WINDOW(updateWindow), GTK_LAYER_SHELL_EDGE_BOTTOM,
                       false);
  gtk_layer_set_anchor(GTK_WINDOW(updateWindow), GTK_LAYER_SHELL_EDGE_LEFT,
                       false);
  gtk_layer_set_anchor(GTK_WINDOW(updateWindow), GTK_LAYER_SHELL_EDGE_RIGHT,
                       false);

  gtk_layer_set_exclusive_zone(GTK_WINDOW(updateWindow), 0);
  gtk_widget_set_opacity(updateWindow, 0.95);

  updateWinGrid = GTK_GRID(gtk_grid_new());
  gtk_widget_set_margin_top(GTK_WIDGET(updateWinGrid), 20);
  gtk_widget_set_margin_bottom(GTK_WIDGET(updateWinGrid), 20);
  gtk_widget_set_margin_start(GTK_WIDGET(updateWinGrid), 20);
  gtk_widget_set_margin_end(GTK_WIDGET(updateWinGrid), 20);
  gtk_container_add(GTK_CONTAINER(updateWindow), GTK_WIDGET(updateWinGrid));

  updateIcon = gtk_image_new();
  gtk_grid_attach(updateWinGrid, updateIcon, 0, 0, 1, 4);

  updateMsg = gtk_label_new("");
  gtk_widget_set_margin_top(updateMsg, 10);
  gtk_grid_attach(updateWinGrid, updateMsg, 0, 4, 1, 1);
}

int AppContext::showUpdateWindow(UpdateModule module, std::string type,
                                 std::string msg) {
  if (updateTimeoutId != 0) {
    g_source_remove(updateTimeoutId);
  }

  std::string svgPath = "resources/icons/";
  switch (module) {
  case UpdateModule::MPRIS:
    svgPath += "mpris/" + type + ".svg";
    break;
  case UpdateModule::NOTIFICATIONS:
    svgPath += "notifications/" + type + ".svg";
    break;
  case UpdateModule::BLUETOOTH:
    svgPath += "bluetooth/" + type + ".svg";
    break;
  case UpdateModule::SCREENSAVER:
    svgPath += "screensaver/" + type + ".svg";
    break;
  case UpdateModule::PULSEAUDIO:
    svgPath += "pulseaudio/" + type + ".svg";
    break;
  case UpdateModule::WIFI:
    svgPath += "wifi/" + type + ".svg";
    break;
  }

  GError *error = nullptr;
  GdkPixbuf *pixBuf = gdk_pixbuf_new_from_file(svgPath.c_str(), &error);
  if (!pixBuf) {
    logger.LogError(TAG, error->message);
    return -1;
  }
  gtk_image_set_from_pixbuf(GTK_IMAGE(updateIcon), pixBuf);
  g_object_unref(pixBuf);

  gtk_label_set_markup(GTK_LABEL(updateMsg), ("<b>" + msg + "</b>").c_str());

  gtk_widget_show_all(updateWindow);
  updateTimeoutId = g_timeout_add_once(2000, hideUpdateWindow, this);
  return 0;
}

void AppContext::hideUpdateWindow(gpointer user_data) {
  AppContext *self = static_cast<AppContext *>(user_data);
  gtk_widget_hide(self->updateWindow);

  self->updateTimeoutId = 0;
}
