#include "services/header/context.hpp"
#include "gdkmm/display.h"
#include "gdkmm/pixbuf.h"
#include "glib.h"
#include "glibmm/main.h"
#include "glibmm/ustring.h"
#include "gtk/gtk.h"
#include "gtk4-layer-shell.h"
#include "gtkmm/cssprovider.h"
#include "gtkmm/enums.h"
#include "gtkmm/eventcontrollerkey.h"
#include "gtkmm/gestureclick.h"
#include "gtkmm/stylecontext.h"
#include "gtkmm/window.h"
#include "iostream"
#include "services/header/database.hpp"
#include "utils/helper_func.hpp"
#include <sdbus-c++/Error.h>
#include <sdbus-c++/IConnection.h>

#define TAG "AppContext"

AppContext::AppContext() : dbus(),logger(true), dbManager(&logger) {}

DbusSystem::DbusSystem() : sysConn(nullptr), ssnConn(nullptr) {
  try {
    sysConn = sdbus::createSystemBusConnection();
    ssnConn = sdbus::createSessionBusConnection();
  } catch (const sdbus::Error &e) {
    std::cerr << "[Error] Failed to Connect With the DBUS System or Session: "
              << e.what() << std::endl;
    return;
  }
}

void AppContext::initWindows() {
  setupUpdateWindow();
  setupNotifWindow();
  setupCtrlWindow();
}

void AppContext::setupUpdateWindow() {

  auto upWinObj = updateWin.gobj();
  gtk_layer_init_for_window(GTK_WINDOW(upWinObj));
  gtk_layer_set_layer(GTK_WINDOW(upWinObj), GTK_LAYER_SHELL_LAYER_OVERLAY);

  gtk_layer_set_anchor(GTK_WINDOW(upWinObj), GTK_LAYER_SHELL_EDGE_TOP, false);
  gtk_layer_set_anchor(GTK_WINDOW(upWinObj), GTK_LAYER_SHELL_EDGE_BOTTOM,
                       false);
  gtk_layer_set_anchor(GTK_WINDOW(upWinObj), GTK_LAYER_SHELL_EDGE_LEFT, false);
  gtk_layer_set_anchor(GTK_WINDOW(upWinObj), GTK_LAYER_SHELL_EDGE_RIGHT, false);

  gtk_layer_set_exclusive_zone(GTK_WINDOW(upWinObj), 0);
  gtk_widget_set_opacity(GTK_WIDGET(upWinObj), 0.95);

  updateWinGrid.set_margin(20);
  updateWin.set_child(updateWinGrid);

  updateIcon.set_pixel_size(64);
  updateWinGrid.attach(updateIcon, 0, 0, 1, 4);

  updateMsg.set_margin_top(10);
  updateMsg.set_wrap(true);
  updateMsg.set_size_request(200, -1);

  updateWinGrid.attach(updateMsg, 0, 4, 1, 1);
}

void AppContext::setupNotifWindow() {

  auto notifWinObj = notifWin.gobj();
  gtk_layer_init_for_window(GTK_WINDOW(notifWinObj));
  gtk_layer_set_layer(GTK_WINDOW(notifWinObj), GTK_LAYER_SHELL_LAYER_OVERLAY);

  gtk_layer_set_anchor(GTK_WINDOW(notifWinObj), GTK_LAYER_SHELL_EDGE_TOP, true);
  gtk_layer_set_anchor(GTK_WINDOW(notifWinObj), GTK_LAYER_SHELL_EDGE_RIGHT,
                       true);

  gtk_layer_set_margin(GTK_WINDOW(notifWinObj), GTK_LAYER_SHELL_EDGE_TOP, 10);
  gtk_layer_set_margin(GTK_WINDOW(notifWinObj), GTK_LAYER_SHELL_EDGE_RIGHT, 10);

  gtk_layer_set_exclusive_zone(GTK_WINDOW(notifWinObj), 0);

  gtk_window_set_decorated(GTK_WINDOW(notifWinObj), false);
  notifWin.set_size_request(400, -1);

  auto gestClick = Gtk::GestureClick::create();
  notifWin.add_controller(gestClick);
  gestClick->signal_pressed().connect([this](int, double, double) {
    notifWin.hide();
  });


  auto notifEvtBox = Gtk::GestureClick::create();
  Gtk::Box notifBox{Gtk::Orientation::HORIZONTAL, 5};
  notifBox.set_margin(5);

  notifBox.add_controller(notifEvtBox);
  notifWin.set_child(notifBox);

  notifLogo.set_size_request(64, 64);
  auto logoCSSProv = Gtk::CssProvider::create();
  logoCSSProv->load_from_data("picture { border-radius: 20px; }");
  notifLogo.get_style_context()->add_provider(logoCSSProv, GTK_STYLE_PROVIDER_PRIORITY_USER);
  notifBox.append(notifLogo);

  Gtk::Box notifTextBox{Gtk::Orientation::VERTICAL, 5};
  notifBox.append(notifTextBox);

  notifTitle.set_wrap(true);
  notifTitle.set_halign(Gtk::Align::START);
  notifTextBox.append(notifTitle);

  notifBody.set_wrap(true);
  notifBody.set_halign(Gtk::Align::START);
  notifTextBox.append(notifBody);
}

void AppContext::setupCtrlWindow() {
  auto ctrlWinObj = ctrlWin.gobj();
  gtk_layer_init_for_window(GTK_WINDOW(ctrlWinObj));
  gtk_layer_set_keyboard_mode(GTK_WINDOW(ctrlWinObj),
                              GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);

  auto evtCtrl = Gtk::EventControllerKey::create();
  evtCtrl->signal_key_pressed().connect(
      [this](unsigned int keyval, unsigned int, Gdk::ModifierType) {
        return this->handleKeyPress(keyval);
      },
      false);
  ctrlWin.add_controller(evtCtrl);

  gtk_layer_set_anchor(GTK_WINDOW(ctrlWinObj), GTK_LAYER_SHELL_EDGE_TOP, false);
  gtk_layer_set_anchor(GTK_WINDOW(ctrlWinObj), GTK_LAYER_SHELL_EDGE_BOTTOM,
                       false);
  gtk_layer_set_anchor(GTK_WINDOW(ctrlWinObj), GTK_LAYER_SHELL_EDGE_LEFT,
                       false);
  gtk_layer_set_anchor(GTK_WINDOW(ctrlWinObj), GTK_LAYER_SHELL_EDGE_RIGHT,
                       false);

  ctrlWin.set_margin_start(50);
  ctrlWin.set_margin_end(50);
  ctrlWin.set_margin_top(30);
  ctrlWin.set_margin_bottom(30);

  Gtk::Box mainBox{Gtk::Orientation::VERTICAL, 10};

  moduleStk.set_transition_type(Gtk::StackTransitionType::CROSSFADE);
  moduleStk.set_hhomogeneous(false);

  mainBox.append(moduleStk);
  ctrlWin.set_child(mainBox);

  auto provider = Gtk::CssProvider::create();
  provider->load_from_data(".win { "
                           "  background-color: transparent; "
                           "}"
                           ".mainBox { "
                           "  background-color: @theme_bg_color; "
                           "  border: 2px solid @borders; "
                           "  border-radius: 12px; "
                           "  margin: 5px; "
                           "}");

  Gtk::StyleContext::add_provider_for_display(
      Gdk::Display::get_default(), provider, GTK_STYLE_PROVIDER_PRIORITY_USER);

  mainBox.add_css_class("mainBox");
  ctrlWin.add_css_class("win");
}

void AppContext::showCtrlWindow(const std::string &moduleName, gint width,
                                gint height) {

  logger.LogInfo(TAG, "Showing Control Window for Module: " + moduleName);

  Glib::signal_idle().connect_once([this, moduleName, width, height]() {
    moduleStk.set_visible_child(moduleName);
    ctrlWin.set_size_request(width, height);
    ctrlWin.show();

    
    logger.LogDebug(TAG, "Set Visible Child in Stack: " + moduleName +
                             " with Size: " + std::to_string(width) + "x" +
                             std::to_string(height));
  }, Glib::PRIORITY_HIGH);
}

void AppContext::addModule(Gtk::Box &moduleBox, const std::string &moduleName) {
  int width, height;
  moduleBox.set_size_request(width, height);
  logger.LogDebug(TAG, "Adding Module to Stack: " + moduleName +
                           " with Size: " + std::to_string(width) + "x" +
                           std::to_string(height));

  moduleStk.add(moduleBox, moduleName, moduleName);
}

bool AppContext::showUpdateWindow(std::string iconName, std::string msg) {
    logger.LogDebug(TAG, "Showing Update Window: " + iconName + " - " + msg);
  if (updateTimeoutConn.connected()) {
    updateTimeoutConn.disconnect();
  }

  Glib::signal_idle().connect_once([this, iconName, msg]() {
    updateIcon.set_from_icon_name(iconName);
    updateMsg.set_markup("<b>" + msg + "</b>");
    updateWin.show();
  });

  updateTimeoutConn = Glib::signal_timeout().connect(
      [this]() -> bool {
        updateWin.hide();
        return false;
      },
      2000);

  return false;
}

void AppContext::showNotifWindow(Notification &notif, bool dnd) {

  NotificationRecord record{
      .id = notif.id,
      .app_name = notif.app_name,
      .summary = notif.summary,
      .body = notif.body,
      .timestamp = dbManager.getCurrentTimestamp(),
      .logo = notif.icon->copy()
  };  

  if (dnd) {
    autoCloseNotificationCb(dnd, record);
    return;
  }

  // Adding Data to UI Elements
  notifTitle.set_markup("<b>" + record.summary + "</b>");

  if (record.body.size() > 500) {
    record.body = record.body.substr(0, 497) + "...";
  }
  notifBody.set_markup(HelperFunc::ValidString(record.body, false));

  if (notif.icon) {
    notifLogo.set(notif.icon->scale_simple(64, 64, Gdk::InterpType::BILINEAR));
    notifLogo.set_pixel_size(64);
  } else {
    notifLogo.clear();
  }

  Glib::signal_timeout().connect_once(
      [this, dnd, record]() { autoCloseNotificationCb(dnd, record); }, 10000);
  notifWin.show();
}

void AppContext::autoCloseNotificationCb(const bool dnd,
                                         const NotificationRecord &record) {
  // Handles the case where notification is already closed by user interaction
  // before timeout
  if (!notifWin.is_visible() && !dnd) {
    return;
  }

  notifWin.hide();

  // Save to DB that notification was closed due to timeout
  if (!dbManager.insertNotification(record)) {
    logger.LogInfo(TAG, "Saved notification ID: " + record.id +
                            " to database before auto-closing.");
  } else {
    logger.LogError(TAG, "Failed to save notification ID: " + record.id +
                             " to database before auto-closing.");
  }
}

bool AppContext::handleKeyPress(unsigned int keyval) {

  if (keyval == GDK_KEY_Escape) {
    ctrlWin.hide();
    return true;
  }
  return false;
}
