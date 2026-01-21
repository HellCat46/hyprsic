#include "manager.hpp"
#include "../../utils/helper_func.hpp"
#include "dbus/dbus-protocol.h"
#include "dbus/dbus-shared.h"
#include "dbus/dbus.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/types.h>

#define TAG "StatusNotifierManager"

StatusNotifierManager::StatusNotifierManager(AppContext *appCtx) : ctx(appCtx) {
  std::ifstream protoFile("resources/dbus/StatusNotifierWatcher.xml");
  if (protoFile) {
    std::stringstream buffer;
    buffer << protoFile.rdbuf();
    SNWXML = buffer.str();

    protoFile.close();
  }

}

void StatusNotifierManager::setupDBus() {
  int ret = dbus_bus_request_name(
      ctx->dbus.ssnConn, "org.kde.StatusNotifierWatcher",
      DBUS_NAME_FLAG_REPLACE_EXISTING, &(ctx->dbus.ssnErr));

  if (dbus_error_is_set(&(ctx->dbus.ssnErr))) {
    std::string errMsg = "Failed to request name for Notifications: ";
    errMsg += ctx->dbus.ssnErr.message;
    ctx->logging.LogError(TAG, errMsg);
    dbus_error_free(&(ctx->dbus.ssnErr));
    return;
  }

  if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
    ctx->logging.LogError(TAG,
                          "Another Notification Service is already running.");
    return;
  }

  // Add Match Rules for Status Notifier Watcher Interface
  dbus_bus_add_match(
      ctx->dbus.ssnConn,
      "type='method_call',interface='org.kde.StatusNotifierWatcher'",
      &(ctx->dbus.ssnErr));
  dbus_bus_add_match(
      ctx->dbus.ssnConn,
      "type='method_call',interface='org.kde.StatusNotifierWatcher',"
      "member='RegisterStatusNotifierItem',path='/StatusNotifierWatcher'",
      &(ctx->dbus.ssnErr));
  dbus_bus_add_match(
      ctx->dbus.ssnConn,
      "type='method_call',interface='org.kde.StatusNotifierWatcher',"
      "member='RegisterStatusNotifierHost',path='/StatusNotifierWatcher'",
      &(ctx->dbus.ssnErr));
  dbus_bus_add_match(ctx->dbus.ssnConn,
                     "type='signal', sender='org.freedesktop.DBus', "
                     "interface='org.freedesktop.DBus', "
                     "member='NameOwnerChanged',path='/org/freedesktop/DBus'",
                     &ctx->dbus.ssnErr);
  if (dbus_error_is_set(&(ctx->dbus.ssnErr))) {
    std::string errMsg = "Failed to add filter for Notifications: ";
    errMsg += ctx->dbus.ssnErr.message;
    ctx->logging.LogError(TAG, errMsg);
    dbus_error_free(&(ctx->dbus.ssnErr));
    return;
  }
  
  
  ctx->logging.LogDebug(TAG, "Started Status Notifier Capture Service");
}

void StatusNotifierManager::handleDbusMessage(DBusMessage *msg) {
  const char *interface = dbus_message_get_interface(msg);
  const char *path = dbus_message_get_path(msg);
  const char *member = dbus_message_get_member(msg);

  if (HelperFunc::saferStrCmp(interface,
                              "org.freedesktop.DBus.Introspectable") &&
      HelperFunc::saferStrCmp(member, "Introspect") &&
      HelperFunc::saferStrCmp(path, "/StatusNotifierWatcher")) {

    handleIntrospectCall(msg);
  } else if (HelperFunc::saferStrCmp(interface,
                                     "org.freedesktop.DBus.Properties") &&
             HelperFunc::saferStrCmp(path, "/StatusNotifierWatcher")) {

    if (HelperFunc::saferStrCmp(member, "GetAll"))
      handleGetAllPropertiesCall(msg);
    else if (HelperFunc::saferStrCmp(member, "Get"))
      handleGetPropertyCall(msg);

  } else if (HelperFunc::saferStrCmp(interface,
                                     "org.kde.StatusNotifierWatcher") &&
             HelperFunc::saferStrCmp(path, "/StatusNotifierWatcher")) {

    if (HelperFunc::saferStrCmp(member, "RegisterStatusNotifierItem"))
      handleRegisterStatusNotifierItem(msg);
    else if (HelperFunc::saferStrCmp(member, "RegisterStatusNotifierHost"))
      handleRegisterStatusNotifierHost(msg);

  } else if (HelperFunc::saferStrCmp(interface, "org.freedesktop.DBus") &&
             HelperFunc::saferStrCmp(member, "NameOwnerChanged") &&
             HelperFunc::saferStrCmp(path, "/org/freedesktop/DBus")) {

    handleNameOwnerChangedSignal(msg);
  }
}

/*
 * Respond to Introspect calls
 * To provide info about the Status Notifier Watcher interface
 * supported Methods, Signals and Properties
 */
void StatusNotifierManager::handleIntrospectCall(DBusMessage *msg) {
  const char *data = SNWXML.c_str();

  DBusMessage *reply = dbus_message_new_method_return(msg);
  dbus_message_append_args(reply, DBUS_TYPE_STRING, &data, DBUS_TYPE_INVALID);
  dbus_connection_send(ctx->dbus.ssnConn, reply, nullptr);
  dbus_connection_flush(ctx->dbus.ssnConn);
  dbus_message_unref(reply);

  ctx->logging.LogDebug(TAG, "Responded to Introspect Call.");
}

void StatusNotifierManager::handleGetAllPropertiesCall(DBusMessage *msg) {
  DBusMessage *reply = dbus_message_new_method_return(msg);
  DBusMessageIter replyIter, arrayIter, dictIter, variantIter;
  dbus_message_iter_init_append(reply, &replyIter);
  dbus_message_iter_open_container(&replyIter, DBUS_TYPE_ARRAY, "{sv}",
                                   &arrayIter);
  dbus_message_iter_close_container(&replyIter, &arrayIter);

  dbus_connection_send(ctx->dbus.ssnConn, reply, nullptr);
  dbus_connection_flush(ctx->dbus.ssnConn);
  dbus_message_unref(reply);

  ctx->logging.LogDebug(TAG, "Received GetAll Properties Call.");
}

void StatusNotifierManager::handleGetPropertyCall(DBusMessage *msg) {
  DBusMessageIter iter;
  dbus_message_iter_init(msg, &iter);

  char *propertyName;
  dbus_message_iter_get_basic(&iter, &propertyName);
  if (!HelperFunc::saferStrCmp(propertyName, "org.kde.StatusNotifierWatcher"))
    return;

  dbus_message_iter_next(&iter);
  dbus_message_iter_get_basic(&iter, &propertyName);

  DBusMessage *reply = dbus_message_new_method_return(msg);
  DBusMessageIter replyIter;
  if (HelperFunc::saferStrCmp(propertyName, "RegisteredStatusNotifierItems")) {
    dbus_message_iter_init_append(reply, &replyIter);
    DBusMessageIter arrayIter;
    dbus_message_iter_open_container(&replyIter, DBUS_TYPE_ARRAY, "s",
                                     &arrayIter);

    for (const auto &item : registeredItems) {
      const char *itemPath = item.first.c_str();
      dbus_message_iter_append_basic(&arrayIter, DBUS_TYPE_STRING, &itemPath);
    }

    dbus_message_iter_close_container(&replyIter, &arrayIter);
  } else if (HelperFunc::saferStrCmp(propertyName,
                                     "IsStatusNotifierHostRegistered")) {
    dbus_message_iter_init_append(reply, &replyIter);

    DBusMessageIter variantIter;
    dbus_bool_t res = true;

    dbus_message_iter_open_container(&replyIter, DBUS_TYPE_VARIANT, "b",
                                     &variantIter);
    dbus_message_iter_append_basic(&variantIter, DBUS_TYPE_BOOLEAN, &res);
    dbus_message_iter_close_container(&replyIter, &variantIter);
  } else if (HelperFunc::saferStrCmp(propertyName, "ProtocolVersion")) {
    dbus_uint32_t version = 0;
    dbus_message_iter_init_append(reply, &replyIter);
    dbus_message_iter_append_basic(&replyIter, DBUS_TYPE_UINT32, &version);
  }

  dbus_message_set_sender(reply, "org.kde.StatusNotifierWatcher");
  dbus_connection_send(ctx->dbus.ssnConn, reply, nullptr);
  dbus_connection_flush(ctx->dbus.ssnConn);
  dbus_message_unref(reply);
}

void StatusNotifierManager::handleRegisterStatusNotifierHost(DBusMessage *msg) {
  DBusMessage *reply = dbus_message_new_method_return(msg);
  dbus_connection_send(ctx->dbus.ssnConn, reply, nullptr);
  dbus_connection_flush(ctx->dbus.ssnConn);
  dbus_message_unref(reply);

  ctx->logging.LogDebug(TAG, "Registered Status Notifier Host.");
}

/* Function will handle Registration of New Apps into Status Notifier Items
 * The Function will extract Dbus Service Address from the Msg
 * and Insert it into registeredItems Set
 * Then it will Emit Signals to notify Clients about the New Item
 * Signal Emitted: StatusNotifierItemRegistered, PropertiesChanged
 */
void StatusNotifierManager::handleRegisterStatusNotifierItem(DBusMessage *msg) {
  DBusMessageIter iter;
  dbus_message_iter_init(msg, &iter);

  char *itemService;
  dbus_message_iter_get_basic(&iter, &itemService);
  std::string itemServiceStr = std::string(itemService);

  // Send Method Return Reply
  DBusMessage *reply = dbus_message_new_method_return(msg);
  if (!reply) {
    ctx->logging.LogError(
        TAG, "Failed to create a Method Return Reply for Item Registration.");
    return;
  }
  dbus_connection_send(ctx->dbus.ssnConn, reply, nullptr);
  dbus_connection_flush(ctx->dbus.ssnConn);
  dbus_message_unref(reply);

  // Emit Signals for Item Registration
  DBusMessage *regSignal = dbus_message_new_signal(
      "/StatusNotifierWatcher", "org.kde.StatusNotifierWatcher",
      "StatusNotifierItemRegistered");
  if (!regSignal) {
    ctx->logging.LogError(
        TAG, "Failed to create StatusNotifierItemRegistered Signal.");
    return;
  }

  DBusMessageIter regSignalIter;
  itemServiceStr += "/StatusNotifierItem";
  dbus_message_iter_init_append(regSignal, &regSignalIter);

  const char *itemPath = itemServiceStr.c_str();
  dbus_message_iter_append_basic(&regSignalIter, DBUS_TYPE_STRING, &itemPath);
  dbus_connection_send(ctx->dbus.ssnConn, regSignal, nullptr);
  dbus_connection_flush(ctx->dbus.ssnConn);
  dbus_message_unref(regSignal);

  // Emit PropertiesChanged Signal for RegisteredStatusNotifierItems Property
  DBusMessage *chgSignal = dbus_message_new_signal(
      "/StatusNotifierWatcher", "org.freedesktop.DBus.Properties",
      "PropertiesChanged");
  if (!chgSignal) {
    ctx->logging.LogError(TAG, "Failed to create PropertiesChanged Signal.");
    return;
  }

  DBusMessageIter chgSignalIter;
  dbus_message_iter_init_append(chgSignal, &chgSignalIter);

  const char *ifaceName = "org.kde.StatusNotifierWatcher";
  dbus_message_iter_append_basic(&chgSignalIter, DBUS_TYPE_STRING, &ifaceName);

  DBusMessageIter dictArrayIter, entryIter, variantIter, itemsArrayIter;
  dbus_message_iter_open_container(&chgSignalIter, DBUS_TYPE_ARRAY, "{sv}",
                                   &dictArrayIter);
  dbus_message_iter_open_container(&dictArrayIter, DBUS_TYPE_DICT_ENTRY,
                                   nullptr, &entryIter);

  const char *propName = "RegisteredStatusNotifierItems";
  dbus_message_iter_append_basic(&entryIter, DBUS_TYPE_STRING, &propName);

  dbus_message_iter_open_container(&entryIter, DBUS_TYPE_VARIANT, "as",
                                   &variantIter);
  dbus_message_iter_open_container(&variantIter, DBUS_TYPE_ARRAY, "s",
                                   &itemsArrayIter);
  for (const auto &item : registeredItems) {
    std::string itemPathStr = item.first + "/StatusNotifierItem";
    const char *itemPathCStr = itemPathStr.c_str();
    dbus_message_iter_append_basic(&itemsArrayIter, DBUS_TYPE_STRING,
                                   &itemPathCStr);
  }

  dbus_message_iter_close_container(&variantIter, &itemsArrayIter);
  dbus_message_iter_close_container(&entryIter, &variantIter);
  dbus_message_iter_close_container(&dictArrayIter, &entryIter);
  dbus_message_iter_close_container(&chgSignalIter, &dictArrayIter);

  DBusMessageIter emptyArrayIter;
  dbus_message_iter_open_container(&chgSignalIter, DBUS_TYPE_ARRAY, "s",
                                   &emptyArrayIter);
  dbus_message_iter_close_container(&chgSignalIter, &emptyArrayIter);

  dbus_connection_send(ctx->dbus.ssnConn, chgSignal, nullptr);
  dbus_connection_flush(ctx->dbus.ssnConn);
  dbus_message_unref(chgSignal);

  // Remove suffix from the service string
  itemServiceStr = itemServiceStr.substr(0, itemServiceStr.rfind("/"));

  StatusApp appInfo;
  getItemInfo(itemServiceStr, appInfo);
  getMenuActions(itemServiceStr, appInfo);

  registeredItems.insert({itemServiceStr, appInfo});

  ctx->logging.LogDebug(TAG, "Registered Status Notifier Item. New Count: " +
                                 std::to_string(registeredItems.size()));
}

// Collect Basic Info from the Status Notifier Item
void StatusNotifierManager::getItemInfo(const std::string &itemService,
                                        StatusApp &outApp) {
  DBusMessage *msg =
      dbus_message_new_method_call(itemService.c_str(), "/StatusNotifierItem",
                                   "org.freedesktop.DBus.Properties", "GetAll");
  if (!msg) {
    ctx->logging.LogError(TAG, "Failed to create a message to get Item Info.");
    return;
  }

  DBusMessageIter args;
  const char *iface = "org.kde.StatusNotifierItem";
  dbus_message_iter_init_append(msg, &args);
  dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &iface);

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.ssnConn, msg, -1, &(ctx->dbus.ssnErr));
  if (!reply && dbus_error_is_set(&(ctx->dbus.ssnErr))) {
    std::string errMsg = "Failed to get a reply for Item Info. ";
    errMsg += ctx->dbus.ssnErr.message;
    ctx->logging.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.ssnErr);
    dbus_message_unref(msg);
    return;
  }

  DBusMessageIter arrayIter, itemIter;
  dbus_message_iter_init(reply, &arrayIter);
  dbus_message_iter_recurse(&arrayIter, &itemIter);

  while (dbus_message_iter_get_arg_type(&itemIter) != DBUS_TYPE_INVALID) {
    DBusMessageIter dictEntryIter, variantIter;
    dbus_message_iter_recurse(&itemIter, &dictEntryIter);

    char *propName;
    dbus_message_iter_get_basic(&dictEntryIter, &propName);

    dbus_message_iter_next(&dictEntryIter);
    dbus_message_iter_recurse(&dictEntryIter, &variantIter);

    if (HelperFunc::saferStrCmp(propName, "IconPixmap")) {
      DBusMessageIter structIter, pixmapDataIter;
      dbus_message_iter_recurse(&variantIter, &structIter);
      dbus_message_iter_recurse(&structIter, &pixmapDataIter);

      int width, height;
      dbus_message_iter_get_basic(&pixmapDataIter, &width);
      dbus_message_iter_next(&pixmapDataIter);
      dbus_message_iter_get_basic(&pixmapDataIter, &height);
      dbus_message_iter_next(&pixmapDataIter);

      DBusMessageIter arrayIter;
      dbus_message_iter_recurse(&pixmapDataIter, &arrayIter);

      int arrLen;
      unsigned char *pixmapData;
      dbus_message_iter_get_fixed_array(&arrayIter, &pixmapData, &arrLen);

      outApp.pixmap =
          gdk_pixbuf_new_from_data(pixmapData, GDK_COLORSPACE_RGB, true, 8,
                                   width, height, width * 4, nullptr, nullptr);

    } else if (HelperFunc::saferStrCmp(propName, "Menu")) {
      char *menuPath;
      dbus_message_iter_get_basic(&variantIter, &menuPath);
      outApp.menu_path = std::string(menuPath);
    } else if (HelperFunc::saferStrCmp(propName, "Status")) {
      char *status;
      dbus_message_iter_get_basic(&variantIter, &status);
      outApp.status = std::string(status);
    }

    dbus_message_iter_next(&itemIter);
  }

  dbus_message_unref(msg);
}

// Collect Menu Actions from the Status Notifier Item to show in Context Menu
void StatusNotifierManager::getMenuActions(const std::string &itemService,
                                           StatusApp &outApp) {
  if (outApp.menu_path.size() == 0) {
    ctx->logging.LogDebug(TAG, "No Menu Path available for the Item. Skipping "
                               "Menu Actions retrieval.");
    return;
  }

  std::string iface = outApp.menu_path;
  std::replace(iface.begin(), iface.end(), '/', '.');
  if (iface[0] == '.')
    iface = iface.substr(1); // Remove leading dot

  DBusMessage *msg = dbus_message_new_method_call(itemService.c_str(),
                                                  outApp.menu_path.c_str(),
                                                  iface.c_str(), "GetLayout");
  if (!msg) {
    ctx->logging.LogError(TAG,
                          "Failed to create a message to get Menu Actions.");
    return;
  }

  DBusMessageIter args, arrayArgs;
  dbus_message_iter_init_append(msg, &args);

  int value = 0;
  dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &value);
  value = -1;
  dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &value);

  dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &arrayArgs);

  const char *propName = "label";
  dbus_message_iter_append_basic(&arrayArgs, DBUS_TYPE_STRING, &propName);
  propName = "type";
  dbus_message_iter_append_basic(&arrayArgs, DBUS_TYPE_STRING, &propName);
  propName = "visible";
  dbus_message_iter_append_basic(&arrayArgs, DBUS_TYPE_STRING, &propName);
  propName = "enabled";
  dbus_message_iter_append_basic(&arrayArgs, DBUS_TYPE_STRING, &propName);
  propName = "children-display";
  dbus_message_iter_append_basic(&arrayArgs, DBUS_TYPE_STRING, &propName);
  propName = "accessible-desc";
  dbus_message_iter_append_basic(&arrayArgs, DBUS_TYPE_STRING, &propName);

  dbus_message_iter_close_container(&args, &arrayArgs);

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.ssnConn, msg, -1, &(ctx->dbus.ssnErr));
  if (!reply && dbus_error_is_set(&(ctx->dbus.ssnErr))) {
    std::string errMsg = "Failed to get a reply for Menu Actions. ";
    errMsg += ctx->dbus.ssnErr.message;
    ctx->logging.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.ssnErr);
    dbus_message_unref(msg);
    return;
  }

  DBusMessageIter replyArgs;
  dbus_message_iter_init(reply, &replyArgs);
  dbus_message_iter_next(&replyArgs);

  DBusMessageIter structIter;
  dbus_message_iter_recurse(&replyArgs, &structIter);
  dbus_message_iter_next(&structIter); // Skip first element
  dbus_message_iter_next(&structIter); // Skip second element (children-display)

  DBusMessageIter arrayIter;
  dbus_message_iter_recurse(&structIter, &arrayIter);

  while (dbus_message_iter_get_arg_type(&arrayIter) != DBUS_TYPE_INVALID) {
    DBusMessageIter menuIter, itemIter;
    dbus_message_iter_recurse(&arrayIter, &menuIter);
    dbus_message_iter_recurse(&menuIter, &itemIter);

    int index;
    dbus_message_iter_get_basic(&itemIter, &index);
    dbus_message_iter_next(&itemIter);

    DBusMessageIter dictIter;
    dbus_message_iter_recurse(&itemIter, &dictIter);

    MenuActionItem menuItem{0, "", true, true, false};
    while (dbus_message_iter_get_arg_type(&dictIter) != DBUS_TYPE_INVALID) {
      DBusMessageIter propIter;
      dbus_message_iter_recurse(&dictIter, &propIter);

      char *propName;
      dbus_message_iter_get_basic(&propIter, &propName);
      dbus_message_iter_next(&propIter);

      DBusMessageIter propValueIter;
      dbus_message_iter_recurse(&propIter, &propValueIter);

      if (HelperFunc::saferStrCmp(propName, "label")) {

        char *label;
        dbus_message_iter_get_basic(&propValueIter, &label);
        menuItem.label = label;
      } else if (HelperFunc::saferStrCmp(propName, "visible")) {

        dbus_bool_t vis;
        dbus_message_iter_get_basic(&propValueIter, &vis);
        menuItem.visible = vis;
      } else if (HelperFunc::saferStrCmp(propName, "enabled")) {

        dbus_bool_t en;
        dbus_message_iter_get_basic(&propValueIter, &en);
        menuItem.enabled = en;
      } else if (HelperFunc::saferStrCmp(propName, "type")) {

        char *type;
        dbus_message_iter_get_basic(&propValueIter, &type);

        if (HelperFunc::saferStrCmp(type, "separator")) {
          menuItem.isSeparator = true;
          break;
        }
      }

      dbus_message_iter_next(&dictIter);
    }

    outApp.menuActions.insert({index, menuItem});
    dbus_message_iter_next(&arrayIter);
  }

  dbus_message_unref(msg);
}

void StatusNotifierManager::handleNameOwnerChangedSignal(DBusMessage *msg) {
  DBusMessageIter args;
  dbus_message_iter_init(msg, &args);

  const char *name, *oldOwner, *newOwner;
  dbus_message_iter_get_basic(&args, &name);
  dbus_message_iter_next(&args);
  dbus_message_iter_get_basic(&args, &oldOwner);
  dbus_message_iter_next(&args);
  dbus_message_iter_get_basic(&args, &newOwner);

  std::string nameStr = std::string(name);
  if (registeredItems.find(nameStr) != registeredItems.end() &&
      std::strlen(newOwner) == 0) {
    registeredItems.erase(nameStr);

    for (auto &callback : removeCallbacks) {
      callback.callback(nameStr, callback.sniApps, callback.widget);
    }
    ctx->logging.LogDebug(TAG,
                          "Unregistered Status Notifier Item. New Count: " +
                              std::to_string(registeredItems.size()));
  }
}

void StatusNotifierManager::executeMenuAction(const std::string &itemService,
                                              const std::string &menuPath,
                                              u_int32_t timestamp,
                                              int actionIndex) {
  std::string iface = menuPath;
  std::replace(iface.begin(), iface.end(), '/', '.');
  if (iface[0] == '.')
    iface = iface.substr(1); // Remove leading dot

  DBusMessage *msg = dbus_message_new_method_call(
      itemService.c_str(), menuPath.c_str(), iface.c_str(), "EventGroup");
  if (!msg) {
    ctx->logging.LogError(TAG,
                          "Failed to create a message to execute Menu Action.");
    return;
  }

  DBusMessageIter args, arrayArgs, structArgs;
  dbus_message_iter_init_append(msg, &args);
  dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "(isvu)",
                                   &arrayArgs);
  dbus_message_iter_open_container(&arrayArgs, DBUS_TYPE_STRUCT, nullptr,
                                   &structArgs);
  dbus_uint32_t actionId = actionIndex;
  dbus_message_iter_append_basic(&structArgs, DBUS_TYPE_INT32, &actionId);
  const char *evtName = "clicked";
  dbus_message_iter_append_basic(&structArgs, DBUS_TYPE_STRING, &evtName);

  DBusMessageIter evtDataVariantIter;
  dbus_int16_t evtData = 0;
  dbus_message_iter_open_container(&structArgs, DBUS_TYPE_VARIANT, "n",
                                   &evtDataVariantIter);
  dbus_message_iter_append_basic(&evtDataVariantIter, DBUS_TYPE_INT16,
                                 &evtData);
  dbus_message_iter_close_container(&structArgs, &evtDataVariantIter);

  dbus_message_iter_append_basic(&structArgs, DBUS_TYPE_UINT32, &timestamp);
  dbus_message_iter_close_container(&arrayArgs, &structArgs);
  dbus_message_iter_close_container(&args, &arrayArgs);

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.ssnConn, msg, -1, &(ctx->dbus.ssnErr));
  if (!reply && dbus_error_is_set(&(ctx->dbus.ssnErr))) {
    std::string errMsg = "Failed to get a reply for Execute Menu Action. ";
    errMsg += ctx->dbus.ssnErr.message;
    ctx->logging.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.ssnErr);
    dbus_message_unref(msg);
    return;
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);
}
