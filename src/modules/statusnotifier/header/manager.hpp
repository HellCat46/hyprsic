#pragma once

#include "gdkmm/pixbuf.h"
#include "gtkmm/image.h"
#include "gtkmm/popover.h"
#include "services/header/comm_types.hpp"
#include "services/header/context.hpp"
#include <functional>
#include <map>
#include <memory>
#include <sdbus-c++/IObject.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Message.h>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <variant>
#include <vector>

struct SNIExecuteMenuAction {
  std::string itemService, menuPath;
  u_int32_t timestamp, actionIndex;

  ModuleType moduleType;
  uint64_t correlationId;
};

struct MenuActionItem {
  int index;
  std::string label;
  bool visible;
  bool enabled;
  bool isSeparator;
};

struct StatusApp {
  std::string menu_path, status;
  Glib::RefPtr<Gdk::Pixbuf> pixmap;
  std::map<int, MenuActionItem> menuActions;

  std::unique_ptr<sdbus::IProxy> dbusProxy;
};

struct SNIApp {
  Gtk::Image icon;
  Gtk::Popover popOver{};
  Gtk::Box parentBox{};
};

struct RemoveCallback {
  std::function<void(std::string servicePath,
                     std::map<std::string, SNIApp> *sniApps, GtkWidget *sniBox)>
      callback;
  GtkWidget *widget;
  std::map<std::string, SNIApp> *sniApps;
};

using SNIRequest = std::variant<SNIExecuteMenuAction>;
class StatusNotifierManager {
  AppContext *ctx;
  std::unique_ptr<sdbus::IObject> dbusObj;
  std::string SNWXML;
  std::vector<RemoveCallback> removeCallbacks;

  // Functions to get Additional Info about Registered Items
  void getItemInfo(StatusApp &outApp);
  void getMenuActions(StatusApp &outApp);

  // Functions to perform the action
  ResponseMessage executeMenuAction(const SNIExecuteMenuAction &action);

  // DBus Message Handler for Status Notifier Watcher Interface
  void handleRegisterStatusNotifierHost(sdbus::MethodCall& msg);
  void handleRegisterStatusNotifierItem(sdbus::MethodCall& msg);

public:
  std::map<std::string, StatusApp> registeredItems;

  StatusNotifierManager(AppContext *appCtx);

  ResponseMessage handle(const SNIRequest &req);
  
  // DBus Introspectable and Properties Handler
  void handleIntrospectCallDbus(sdbus::Message& msg);
  void handleGetAllPropertiesCallDbus(sdbus::Message& msg);
  void handleGetPropertyCallDbus(sdbus::Message& msg);
  void handleNameOwnerChangedSignalDbus(std::string& name, std::string_view newOwner);
};
