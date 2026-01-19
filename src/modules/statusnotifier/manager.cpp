#include "manager.hpp"
#include "dbus/dbus-protocol.h"
#include "dbus/dbus-shared.h"
#include "dbus/dbus.h"
#include "../../utils/helper_func.hpp"
#include <cstring>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

#define TAG "StatusNotifierManager"

StatusNotifierManager::StatusNotifierManager(AppContext *appCtx) : ctx(appCtx) {
  std::ifstream protoFile("resources/dbus/StatusNotifierWatcher.xml");
  if (protoFile) {
    std::stringstream buffer;
    buffer << protoFile.rdbuf();
    SNWXML = buffer.str();

    protoFile.close();
  }

  ssnThread = std::thread(&StatusNotifierManager::captureStatusNotifier, this);
}

void StatusNotifierManager::captureStatusNotifier() {
  ctx->logging.LogDebug(TAG, "Starting Status Notifier Capture Service");
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
  if (dbus_error_is_set(&(ctx->dbus.ssnErr))) {
    std::string errMsg = "Failed to add filter for Notifications: ";
    errMsg += ctx->dbus.ssnErr.message;
    ctx->logging.LogError(TAG, errMsg);
    dbus_error_free(&(ctx->dbus.ssnErr));
    return;
  }

  DBusMessage *msg;
  while (1) {
    if (!dbus_connection_read_write_dispatch(ctx->dbus.ssnConn, 100)) {
      ctx->logging.LogError(
          TAG, "Connection Closed while Waiting for Notification Messages");
      return;
    }

    msg = dbus_connection_pop_message(ctx->dbus.ssnConn);
    if (!msg) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    const char *interface = dbus_message_get_interface(msg);
    const char *member = dbus_message_get_member(msg);
    const char *path = dbus_message_get_path(msg);

    // Respond to Introspect calls
    // To provide info about the Status Notifier Watcher interface
    // supported Methods, Signals and Properties
    if (HelperFunc::saferStrCmp(interface, "org.freedesktop.DBus.Introspectable") &&
        HelperFunc::saferStrCmp(member, "Introspect") &&
        HelperFunc::saferStrCmp(path, "/StatusNotifierWatcher")) {
      handleIntrospectCall(msg);
    } else if (HelperFunc::saferStrCmp(interface, "org.freedesktop.DBus.Properties") &&
               HelperFunc::saferStrCmp(member, "GetAll") &&
               HelperFunc::saferStrCmp(path, "/StatusNotifierWatcher")) {
      handleGetAllPropertiesCall(msg);
    }

    
    dbus_message_unref(msg);
    ctx->logging.LogDebug(TAG, "Received Status Notifier Message. " +
                                  std::string(interface) + "." +
                                  std::string(member) + " " + std::string(path));
  }
}

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
