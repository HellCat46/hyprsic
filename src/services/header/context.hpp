#pragma once
#include "database.hpp"
#include "gdkmm/pixbuf.h"
#include "glib.h"
#include "gtkmm/box.h"
#include "gtkmm/grid.h"
#include "gtkmm/image.h"
#include "gtkmm/label.h"
#include "gtkmm/stack.h"
#include "gtkmm/window.h"
#include "sdbus-c++/sdbus-c++.h"
#include "services/header/logging.hpp"
#include <sdbus-c++/IConnection.h>
#include <string>
#include <vector>

class DbusSystem {
public:
  std::unique_ptr<sdbus::IConnection> sysConn;
  std::unique_ptr<sdbus::IConnection> ssnConn;

  DbusSystem();
};

struct Notification {
  std::string id, app_name;
  uint32_t replaces_id;
  std::string app_icon, summary, body;
  int32_t expire_timeout;

  std::vector<std::string> actions;
  std::map<std::string, sdbus::Variant> hints;
  Glib::RefPtr<Gdk::Pixbuf> icon;
};

struct NotifListItem {
  std::list<NotificationRecord>::iterator it;
  Gtk::Box widget{Gtk::Orientation::HORIZONTAL, 5};
};

class AppContext {
  Gtk::Window updateWin;
  Gtk::Window ctrlWin;
  Gtk::Window notifWin;

  // Notification Window Manager
  Gtk::Image notifLogo;
  Gtk::Label notifTitle;
  Gtk::Label notifBody;

  Gtk::Grid updateWinGrid;
  Gtk::Image updateIcon;
  Gtk::Label updateMsg;
  sigc::connection updateTimeoutConn;

  bool handleKeyPress(unsigned int keyval);

  // Notification Window Functions
  void autoCloseNotificationCb(const bool dnd, const NotificationRecord& record);

  // Setup Windows
  void setupUpdateWindow();
  void setupNotifWindow();
  void setupCtrlWindow();

public:
  DbusSystem dbus;
  LoggingManager logger;
  DBManager dbManager;
  Gtk::Stack moduleStk;

  AppContext();
  void initWindows();
  bool showUpdateWindow(std::string type, std::string msg);
  void showCtrlWindow(const std::string &moduleName, gint width = -1,
                      gint height = -1);
  void showNotifWindow(Notification& notif, bool dnd);
  void addModule(Gtk::Box &moduleBox, const std::string &moduleName);
};