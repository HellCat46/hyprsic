#include "manager.hpp"
#include "../../utils/helper_func.hpp"
#include "dbus/dbus-protocol.h"
#include "dbus/dbus-shared.h"
#include "dbus/dbus.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "glib.h"
#include <cstring>

#define TAG "NotificationManager"

NotificationManager::NotificationManager(AppContext *ctx) : ctx(ctx) {}

void NotificationManager::setupDBus() {
  int ret = dbus_bus_request_name(
      ctx->dbus.ssnConn, "org.freedesktop.Notifications",
      DBUS_NAME_FLAG_REPLACE_EXISTING, &(ctx->dbus.ssnErr));

  if (dbus_error_is_set(&(ctx->dbus.ssnErr))) {
    std::string errMsg = "Failed to request name for Notifications: ";
    errMsg += ctx->dbus.ssnErr.message;
    ctx->logger.LogError(TAG, errMsg);
    dbus_error_free(&(ctx->dbus.ssnErr));
    return;
  }

  if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
    ctx->logger.LogError(TAG,
                          "Another Notification Service is already running.");
    return;
  }

  dbus_bus_add_match(
      ctx->dbus.ssnConn,
      "type='method_call',interface='org.freedesktop.Notifications'",
      &(ctx->dbus.ssnErr));
  dbus_bus_add_match(
      ctx->dbus.ssnConn,
      "type='method_call',interface='org.freedesktop.Notifications',"
      "member='Notify',path='/org/freedesktop/Notifications'",
      &(ctx->dbus.ssnErr));

  if (dbus_error_is_set(&(ctx->dbus.ssnErr))) {
    std::string errMsg = "Failed to add filter for Notifications: ";
    errMsg += ctx->dbus.ssnErr.message;
    ctx->logger.LogError(TAG, errMsg);
    dbus_error_free(&(ctx->dbus.ssnErr));
    return;
  }

  ctx->logger.LogInfo(TAG, "Started Notification Capture Service");
}

void NotificationManager::handleDbusMessage(
    DBusMessage *msg, std::function<void(NotifFuncArgs *)> showNotification) {
  const char *member = dbus_message_get_member(msg);
  int type = dbus_message_get_type(msg);

  if (type != DBUS_MESSAGE_TYPE_METHOD_CALL)
    return;

  // Method Call Handlers
  if (HelperFunc::saferStrCmp(member, "Notify")) {

    Notification notification = handleNotifyCall(msg);
    if (notification.app_name.size() > 0) {
      NotifFuncArgs args;
      args.notif = &notification;
      args.notifications = &notifications;
      args.logger = &ctx->logger;
      args.dbManager = &ctx->dbManager;

      showNotification(&args);
    }
  } else if (HelperFunc::saferStrCmp(member, "CloseNotification")) {

    handleCloseNotificationCall(msg);
  } else if (HelperFunc::saferStrCmp(member, "GetCapabilities")) {

    handleGetCapabilitiesCall(msg);
  } else if (HelperFunc::saferStrCmp(member, "GetServerInformation")) {

    ctx->logger.LogInfo(TAG, "Received GetServerInformation Call");
    handleGetServerInformationCall(msg);
  }
}

void NotificationManager::handleGetServerInformationCall(DBusMessage *msg) {
  DBusMessage *reply = dbus_message_new_method_return(msg);

  const char *name = "Hyprsic";
  const char *vendor = "Hellcat";
  const char *version = "0.1.0";
  const char *specVersion = "1.2";

  dbus_message_append_args(reply, DBUS_TYPE_STRING, &name, DBUS_TYPE_STRING,
                           &vendor, DBUS_TYPE_STRING, &version,
                           DBUS_TYPE_STRING, &specVersion, DBUS_TYPE_INVALID);

  dbus_connection_send(ctx->dbus.ssnConn, reply, nullptr);
  dbus_connection_flush(ctx->dbus.ssnConn);
  dbus_message_unref(reply);
}

void NotificationManager::handleGetCapabilitiesCall(DBusMessage *msg) {
  DBusMessage *reply = dbus_message_new_method_return(msg);

  const char *capabilities[] = {"body",        "body-hyperlinks",
                                "body-markup", "icon-static",
                                "actions",     "persistence"};

  DBusMessageIter args, array;
  dbus_message_iter_init_append(reply, &args);
  dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &array);

  for (int idx = 0; idx < (sizeof(capabilities) / sizeof(capabilities[0]));
       idx++) {
    const char *cap = capabilities[idx];
    dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &cap);
  }

  dbus_message_iter_close_container(&args, &array);

  dbus_connection_send(ctx->dbus.ssnConn, reply, nullptr);
  dbus_connection_flush(ctx->dbus.ssnConn);
  dbus_message_unref(reply);
}

void NotificationManager::handleCloseNotificationCall(DBusMessage *msg) {
  DBusMessageIter args;
  dbus_int32_t notifId;

  if (!dbus_message_iter_init(msg, &args)) {
    ctx->logger.LogError(TAG, "CloseNotification Message has no arguments!");
    return;
  }

  dbus_message_iter_get_basic(&args, &notifId);

  DBusMessage *reply = dbus_message_new_method_return(msg);
  dbus_connection_send(ctx->dbus.ssnConn, reply, nullptr);
  dbus_connection_flush(ctx->dbus.ssnConn);
  dbus_message_unref(reply);

  DBusMessage *signal = dbus_message_new_signal(
      "/org/freedesktop/Notifications", "org.freedesktop.Notifications",
      "NotificationClosed");

  dbus_uint32_t reason = 2;
  dbus_message_append_args(signal, DBUS_TYPE_UINT32, &notifId, DBUS_TYPE_UINT32,
                           &reason, DBUS_TYPE_INVALID);
  dbus_connection_send(ctx->dbus.ssnConn, signal, nullptr);
  dbus_connection_flush(ctx->dbus.ssnConn);
  dbus_message_unref(signal);
}

Notification NotificationManager::handleNotifyCall(DBusMessage *msg) {
  DBusMessageIter args;
  Notification notif;
  notif.id = g_uuid_string_random();

  if (!dbus_message_iter_init(msg, &args)) {
    ctx->logger.LogError(TAG, "Notification Message has no arguments!");

    DBusMessage *reply = dbus_message_new_error(msg, DBUS_ERROR_INVALID_ARGS,
                                                "No arguments provided");
    dbus_connection_send(ctx->dbus.ssnConn, reply, nullptr);
    dbus_message_unref(reply);
    return notif;
  }

  // Parse Application Name
  const char *app_name = nullptr;
  if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_STRING) {
    ctx->logger.LogError(TAG, "Invalid argument type for app_name!");
    return notif;
  }
  dbus_message_iter_get_basic(&args, &app_name);
  dbus_message_iter_next(&args);
  notif.app_name = app_name;

  // Parse Replaces ID
  dbus_uint32_t replaces_id = 0;
  if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_UINT32) {
    ctx->logger.LogError(TAG, "Invalid argument type for replaces_id!");
    return notif;
  }
  dbus_message_iter_get_basic(&args, &replaces_id);
  dbus_message_iter_next(&args);
  notif.replaces_id = replaces_id;

  // Parse App Icon
  const char *app_icon = nullptr;
  if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_STRING) {
    ctx->logger.LogError(TAG, "Invalid argument type for app_icon!");
    return notif;
  }
  dbus_message_iter_get_basic(&args, &app_icon);
  dbus_message_iter_next(&args);
  notif.app_icon = app_icon;

  // Parse Summary
  const char *summary = nullptr;
  if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_STRING) {
    ctx->logger.LogError(TAG, "Invalid argument type for summary!");
    return notif;
  }
  dbus_message_iter_get_basic(&args, &summary);
  dbus_message_iter_next(&args);
  notif.summary = summary;

  // Parse Body
  const char *body = nullptr;
  if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_STRING) {
    ctx->logger.LogError(TAG, "Invalid argument type for body!");
    return notif;
  }
  dbus_message_iter_get_basic(&args, &body);
  dbus_message_iter_next(&args);
  notif.body = body;

  // Parse Actions
  DBusMessageIter actions_iter;
  if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_ARRAY) {
    ctx->logger.LogError(TAG, "Invalid argument type for actions!");
    return notif;
  }
  dbus_message_iter_recurse(&args, &actions_iter);
  dbus_message_iter_next(&args);

  // Parse Hints
  DBusMessageIter hints_iter;
  if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_ARRAY) {
    ctx->logger.LogError(TAG, "Invalid argument type for hints!");
    return notif;
  }
  dbus_message_iter_recurse(&args, &hints_iter);
  notif.icon_pixbuf = parseImageData(&hints_iter);
  dbus_message_iter_next(&args);

  // Parse Expire Timeout
  dbus_int32_t expire_timeout = 0;
  if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_INT32) {
    ctx->logger.LogError(TAG, "Invalid argument type for expire_timeout!");
    return notif;
  }
  dbus_message_iter_get_basic(&args, &expire_timeout);
  dbus_message_iter_next(&args);

  // std::string notifLog = "Notification from App: ";
  // notifLog += app_name;
  // notifLog += " | Summary: ";
  // notifLog += summary;
  // notifLog += " | Body: ";
  // notifLog += body;
  // notifLog += " | Icon: ";
  // notifLog += app_icon;
  // notifLog += " | Timeout: ";
  // notifLog += std::to_string(expire_timeout);
  // ctx->logger.LogInfo(TAG, notifLog);

  static dbus_uint32_t notifIdCounter = 1;
  dbus_uint32_t notifId = notifIdCounter++;

  DBusMessage *reply = dbus_message_new_method_return(msg);
  dbus_message_append_args(reply, DBUS_TYPE_UINT32, &notifId,
                           DBUS_TYPE_INVALID);

  dbus_connection_send(ctx->dbus.ssnConn, reply, nullptr);
  dbus_connection_flush(ctx->dbus.ssnConn);
  dbus_message_unref(reply);

  return notif;
}

GdkPixbuf *NotificationManager::parseImageData(DBusMessageIter *hintsIter) {
  DBusMessageIter dictIter, variantIter, structIter;

  while (dbus_message_iter_get_arg_type(hintsIter) != DBUS_TYPE_INVALID) {
    dbus_message_iter_recurse(hintsIter, &dictIter);

    const char *key;
    dbus_message_iter_get_basic(&dictIter, &key);
    dbus_message_iter_next(&dictIter);

    if (HelperFunc::saferStrCmp(key, "image-data")) {
      dbus_message_iter_recurse(&dictIter, &variantIter);
      dbus_message_iter_recurse(&variantIter, &structIter);

      int width, height, rowstride, bits_per_sample, channels;
      dbus_bool_t alpha;

      dbus_message_iter_get_basic(&structIter, &width);
      dbus_message_iter_next(&structIter);

      dbus_message_iter_get_basic(&structIter, &height);
      dbus_message_iter_next(&structIter);

      dbus_message_iter_get_basic(&structIter, &rowstride);
      dbus_message_iter_next(&structIter);

      dbus_message_iter_get_basic(&structIter, &alpha);
      dbus_message_iter_next(&structIter);

      dbus_message_iter_get_basic(&structIter, &bits_per_sample);
      dbus_message_iter_next(&structIter);

      dbus_message_iter_get_basic(&structIter, &channels);
      dbus_message_iter_next(&structIter);

      DBusMessageIter arrayIter;
      dbus_message_iter_recurse(&structIter, &arrayIter);

      int arrLen;
      unsigned char *pixelData;
      dbus_message_iter_get_fixed_array(&arrayIter, &pixelData, &arrLen);

      GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data(
          pixelData, GDK_COLORSPACE_RGB, alpha, bits_per_sample, width, height,
          rowstride, nullptr, nullptr);

      if (pixbuf) {
        GdkPixbuf *finalPixbuf = gdk_pixbuf_copy(pixbuf);
        g_object_unref(pixbuf);
        return finalPixbuf;
      }
    }

    dbus_message_iter_next(hintsIter);
  }

  return nullptr;
}
