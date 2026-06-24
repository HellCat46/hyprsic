#include "header/manager.hpp"
#include "gdkmm/pixbuf.h"
#include "services/header/comm_types.hpp"
#include <algorithm>
#include <cstdint>
#include <map>
#include <ranges>
#include <sdbus-c++/Error.h>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/VTableItems.h>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <variant>
#include <vector>

#define TAG "StatusNotifierManager"

StatusNotifierManager::StatusNotifierManager(AppContext *appCtx) : ctx(appCtx) {
  std::ifstream protoFile("resources/dbus/StatusNotifierWatcher.xml");
  if (protoFile) {
    std::stringstream buffer;
    buffer << protoFile.rdbuf();
    SNWXML = buffer.str();

    protoFile.close();
  }

  try {
    ctx->dbus.ssnConn->requestName(
        sdbus::ServiceName{"org.kde.StatusNotifierWatcher"});
  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG,
                         "Another Notification Service is already running." + std::string{e.what()});
    return;
  }

  try {
    dbusObj = sdbus::createObject(*ctx->dbus.ssnConn,
                                  sdbus::ObjectPath{"/StatusNotifierWatcher"});

    dbusObj->addVTable(
        sdbus::InterfaceName{"org.kde.StatusNotifierWatcher"},
        sdbus::MethodVTableItem{sdbus::MethodName{"RegisterStatusNotifierItem"},
                                sdbus::Signature{""},
                                {},
                                {},
                                {},
                                [this](sdbus::MethodCall msg) {
                                  this->handleRegisterStatusNotifierItem(msg);
                                },
                                {}},
        sdbus::MethodVTableItem{sdbus::MethodName{"RegisterStatusNotifierHost"},
                                sdbus::Signature{""},
                                {},
                                {},
                                {},
                                [this](sdbus::MethodCall msg) {
                                  this->handleRegisterStatusNotifierHost(msg);
                                },
                                {}});

    ctx->dbus.ssnConn->addMatch(
        "type='method_call', interface='org.freedesktop.DBus.Introspectable', "
        "member='Introspect', path='/StatusNotifierWatcher'",
        [this](sdbus::Message msg) { handleIntrospectCallDbus(msg); });

    ctx->dbus.ssnConn->addMatch(
        "type='method_call', interface='org.freedesktop.DBus.Properties', "
        "member='GetAll', path='/StatusNotifierWatcher'",
        [this](sdbus::Message msg) { handleGetAllPropertiesCallDbus(msg); });

    ctx->dbus.ssnConn->addMatch(
        "type='method_call', interface='org.freedesktop.DBus.Properties', "
        "member='Get', path='/StatusNotifierWatcher'",
        [this](sdbus::Message msg) { handleGetPropertyCallDbus(msg); });

    ctx->logger.LogDebug(TAG, "Started Status Notifier Capture Service");
  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG, "Failed to request name for Notifications: " +
                                  std::string(e.what()));
  }
}

/*
 * Respond to Introspect calls
 * To provide info about the Status Notifier Watcher interface
 * supported Methods, Signals and Properties
 */
void StatusNotifierManager::handleIntrospectCallDbus(sdbus::Message &msg) {
  try {
    auto msgMethod = static_cast<sdbus::MethodCall &>(msg);
    sdbus::MethodReply reply = msgMethod.createReply();
    reply << SNWXML;
    reply.send();

    ctx->logger.LogDebug(TAG, "Responded to Introspect Call.");
  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG, "Failed to handle Introspect call: " +
                                  std::string(e.what()));
  }
}

void StatusNotifierManager::handleGetAllPropertiesCallDbus(
    sdbus::Message &msg) {
  try {
    sdbus::MethodCall &msgMethod = static_cast<sdbus::MethodCall &>(msg);
    sdbus::MethodReply reply = msgMethod.createReply();

    reply << std::map<std::string, sdbus::Variant>{};
    reply.send();

    ctx->logger.LogDebug(TAG, "Responded to GetAll Properties Call.");
  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG, "Failed to handle GetAll Properties call: " +
                                  std::string(e.what()));
  }
}

void StatusNotifierManager::handleGetPropertyCallDbus(sdbus::Message &msg) {
  try {
    sdbus::MethodCall &msgMethod = static_cast<sdbus::MethodCall &>(msg);
    sdbus::MethodReply reply = msgMethod.createReply();

    std::string iface;
    msg >> iface;
    if (!(iface == "org.kde.StatusNotifierWatcher"))
      return;

    std::string propName;
    msg >> propName;
    if (propName == "RegisteredStatusNotifierItems") {
      auto keys_view = std::ranges::views::keys(registeredItems);
      std::vector<std::string> items(keys_view.begin(), keys_view.end());

      reply << sdbus::Variant{items};
    } else if (propName == "IsStatusNotifierHostRegistered") {
      reply << sdbus::Variant{true};
    } else if (propName == "ProtocolVersion") {
      reply << sdbus::Variant{int32_t{0}};
    }

    reply.send();
  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG, "Failed to handle GetProperty call: " +
                                  std::string(e.what()));
  }
}

void StatusNotifierManager::handleRegisterStatusNotifierHost(
    sdbus::MethodCall &msg) {
  try {
    sdbus::MethodReply reply = msg.createReply();
    reply.send();
    ctx->logger.LogDebug(TAG, "Registered Status Notifier Host.");
  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG,
                         "Failed to handle RegisterStatusNotifierHost call: " +
                             std::string(e.what()));
  }
}

/* Function will handle Registration of New Apps into Status Notifier Items
 * The Function will extract Dbus Service Address from the Msg
 * and Insert it into registeredItems Set
 * Then it will Emit Signals to notify Clients about the New Item
 * Signal Emitted: StatusNotifierItemRegistered, PropertiesChanged
 */
void StatusNotifierManager::handleRegisterStatusNotifierItem(
    sdbus::MethodCall &msg) {

  std::string itemServ;
  try {
    sdbus::MethodReply reply = msg.createReply();
    msg >> itemServ;
    reply.send();
  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG,
                         "Failed to handle RegisterStatusNotifierItem call: " +
                             std::string(e.what()));
    return;
  }

  // Emit Signals for Item Registration
  try {
    sdbus::Signal signal = dbusObj->createSignal(
        sdbus::InterfaceName{"org.kde.StatusNotifierWatcher"},
        sdbus::SignalName{"StatusNotifierItemRegistered"});

    signal << std::string{itemServ + "/StatusNotifierItem"};
    signal.send();
  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(
        TAG, "Failed to emit StatusNotifierItemRegistered signal: " +
                 std::string(e.what()));
    return;
  }

  // Emit PropertiesChanged Signal for RegisteredStatusNotifierItems Property
  try {
    sdbus::Signal signal = dbusObj->createSignal(
        sdbus::InterfaceName{"org.freedesktop.DBus.Properties"},
        sdbus::SignalName{"PropertiesChanged"});

    auto keys_view = std::ranges::views::keys(registeredItems);
    std::vector<std::string> items(keys_view.begin(), keys_view.end());
    signal << std::string{"org.kde.StatusNotifierWatcher"}
           << std::map<std::string,
                       sdbus::Variant>{{"RegisteredStatusNotifierItems",
                                        sdbus::Variant{items}}}
           << std::vector<std::string>{};

    signal.send();

    StatusApp appInfo;
    appInfo.dbusProxy =
        sdbus::createProxy(*ctx->dbus.ssnConn, sdbus::ServiceName{itemServ},
                           sdbus::ObjectPath{"/StatusNotifierItem"});
    getItemInfo(appInfo);
    getMenuActions(appInfo);

    registeredItems.insert({itemServ, std::move(appInfo)});

    ctx->logger.LogDebug(TAG, "Registered Status Notifier Item. New Count: " +
                                  std::to_string(registeredItems.size()));
  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG, "Failed to register Status Notifier Item: " +
                                  std::string(e.what()));
  }
}

// Collect Basic Info from the Status Notifier Item
void StatusNotifierManager::getItemInfo(StatusApp &outApp) {
  sdbus::MethodCall msg = outApp.dbusProxy->createMethodCall(
      sdbus::InterfaceName{"org.freedesktop.DBus.Properties"},
      sdbus::MethodName{"GetAll"});

  msg << std::string{"org.kde.StatusNotifierItem"};

  sdbus::MethodReply reply = outApp.dbusProxy->callMethod(msg);

  std::map<std::string, sdbus::Variant> props;
  reply >> props;

  for (const auto &[propName, propValue] : props) {
    if (propName == "IconPixmap") {
      using PixMap = sdbus::Struct<int32_t, int32_t, std::vector<uint8_t>>;

      auto pixmapArray = propValue.get<std::vector<PixMap>>();

      if (!pixmapArray.empty()) {
        uint32_t width = pixmapArray[0].get<0>();
        uint32_t height = pixmapArray[0].get<1>();
        std::vector<uint8_t> pixmapData = pixmapArray[0].get<2>();

        outApp.pixmap = Gdk::Pixbuf::create_from_data(
            pixmapData.data(), Gdk::Colorspace::RGB, true, 8, width, height,
            width * 4);
      }
    } else if (propName == "Menu") {
      outApp.menu_path = propValue.get<std::string>();
    } else if (propName == "Status") {
      outApp.status = propValue.get<std::string>();
    }
  }
}

// Collect Menu Actions from the Status Notifier Item to show in Context Menu
void StatusNotifierManager::getMenuActions(StatusApp &outApp) {
  if (outApp.menu_path.size() == 0) {
    ctx->logger.LogDebug(TAG, "No Menu Path available for the Item. Skipping "
                              "Menu Actions retrieval.");
    return;
  }

  std::string iface = outApp.menu_path;
  std::replace(iface.begin(), iface.end(), '/', '.');
  if (iface[0] == '.')
    iface = iface.substr(1); // Remove leading dot

  sdbus::MethodCall msg = outApp.dbusProxy->createMethodCall(
      sdbus::InterfaceName{iface}, sdbus::MethodName{"GetLayout"});
  msg << uint32_t{0} << int32_t{-1}
      << std::vector<std::string>{
             "label",          "type", "visible", "enabled", "children-display",
             "accessible-desc"};

  sdbus::MethodReply reply = outApp.dbusProxy->callMethod(msg);

  uint32_t count;
  reply >> count;

  using MenuNode = sdbus::Struct<int32_t, std::map<std::string, sdbus::Variant>,
                                 std::vector<sdbus::Variant>>;
  MenuNode layoutData;

  reply >> layoutData;

  for (const auto &item : layoutData.get<2>()) {
    auto menuItem = item.get<MenuNode>();
    MenuActionItem itemObj{menuItem.get<0>(), "", true, true, false};

    auto menuItemProps = menuItem.get<1>();

    if (menuItemProps.find("label") != menuItemProps.end()) {
      itemObj.label = menuItemProps.at("label").get<std::string>();
    }

    if (menuItemProps.find("visible") != menuItemProps.end()) {
      itemObj.visible = menuItemProps.at("visible").get<bool>();
    }

    if (menuItemProps.find("enabled") != menuItemProps.end()) {
      itemObj.enabled = menuItemProps.at("enabled").get<bool>();
    }

    if (menuItemProps.find("type") != menuItemProps.end()) {
      itemObj.isSeparator = menuItemProps.at("type").get<std::string>() == "separator";
    }

    outApp.menuActions.insert({itemObj.index, itemObj});
  }
  
}

void StatusNotifierManager::handleNameOwnerChangedSignalDbus(
    std::string &name, std::string_view newOwner) {

  if (registeredItems.find(name) != registeredItems.end() &&
      newOwner.length() == 0) {
    registeredItems.erase(name);

    for (auto &callback : removeCallbacks) {
      callback.callback(name, callback.sniApps, callback.widget);
    }
    ctx->logger.LogDebug(TAG, "Unregistered Status Notifier Item. New Count: " +
                                  std::to_string(registeredItems.size()));
  }
}

ResponseMessage
StatusNotifierManager::executeMenuAction(const SNIExecuteMenuAction &action) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = action.correlationId};

  std::string iface = action.menuPath;
  std::replace(iface.begin(), iface.end(), '/', '.');
  if (iface[0] == '.')
    iface = iface.substr(1); // Remove leading dot

  try {
    using EventEntry =
        sdbus::Struct<int32_t, std::string, sdbus::Variant, uint32_t>;

    auto it = registeredItems.find(action.itemService);
    if (it == registeredItems.end()) {
      resp.errMsg = "Item service not found.";
      return resp;
    }

    StatusApp &appInfo = it->second;
    sdbus::MethodCall msg = appInfo.dbusProxy->createMethodCall(
        sdbus::InterfaceName{iface}, sdbus::MethodName{"EventGroup"});

    msg << std::vector<EventEntry>{
        EventEntry{int32_t(action.actionIndex), std::string{"clicked"},
                   sdbus::Variant{0}, uint32_t{action.timestamp}}};

    msg.doesntExpectReply();
    appInfo.dbusProxy->callMethod(msg);

    resp.success = true;
  } catch (const std::exception &e) {
    resp.errMsg = e.what();
    ctx->logger.LogError(TAG, resp.errMsg);
  }

  return resp;
}

ResponseMessage StatusNotifierManager::handle(const SNIRequest &req) {
  ResponseMessage resp;

  std::visit(
      [&](auto &reqMsg) {
        using T = std::decay_t<decltype(reqMsg)>;

        if constexpr (std::is_same_v<T, SNIExecuteMenuAction>) {
          resp = executeMenuAction(reqMsg);
        }
      },
      req);

  return resp;
}
