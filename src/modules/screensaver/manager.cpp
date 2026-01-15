#include "manager.hpp"
#include "dbus/dbus-protocol.h"
#include "dbus/dbus.h"

#define TAG "ScreenSaverManager"

ScreenSaverManager::ScreenSaverManager(AppContext *ctx) { this->ctx = ctx; }

int ScreenSaverManager::activateScreenSaver() {

  DBusMessage *msg = dbus_message_new_method_call(
      "org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver",
      "org.freedesktop.ScreenSaver", "Inhibit");
  if (!msg) {
    this->ctx->logging.LogError(
        TAG, "Failed to create D-Bus message for Screen Saver Inhibit");
    return 1;
  }

  const char *app_name = "Hyprsic";
  const char *reason = "Activating Screen Saver";

  if (!dbus_message_append_args(msg, DBUS_TYPE_STRING, &app_name,
                                DBUS_TYPE_STRING, &reason, DBUS_TYPE_INVALID)) {
    this->ctx->logging.LogError(
        TAG, "Failed to append arguments to Screen Saver Inhibit message");
    dbus_message_unref(msg);
    return 1;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      this->ctx->dbus.ssnConn, msg, -1, &this->ctx->dbus.ssnErr);
  if (!reply && dbus_error_is_set(&this->ctx->dbus.ssnErr)) {
    std::string errMsg = "D-Bus Error on Screen Saver Inhibit: ";
    errMsg += this->ctx->dbus.ssnErr.message;
    this->ctx->logging.LogError(TAG, errMsg.c_str());
    dbus_error_free(&this->ctx->dbus.ssnErr);
    dbus_message_unref(msg);
    return 1;
  }

  DBusMessageIter iter;
  if (!dbus_message_iter_init(reply, &iter)) {
    this->ctx->logging.LogError(TAG,
                                "Screen Saver Inhibit reply has no arguments");
    dbus_message_unref(msg);
    dbus_message_unref(reply);
    return 1;
  }
  dbus_message_iter_get_basic(&iter, &inhibitCookie);

  dbus_message_unref(msg);
  dbus_message_unref(reply);
  return 0;
}

int ScreenSaverManager::deactivateScreenSaver() {
  DBusMessage *msg = dbus_message_new_method_call(
      "org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver",
      "org.freedesktop.ScreenSaver", "UnInhibit");
  if (!msg) {
    this->ctx->logging.LogError(
        TAG, "Failed to create D-Bus message for Screen Saver UnInhibit");
    return 1;
  }

  if (!dbus_message_append_args(msg, DBUS_TYPE_UINT32, &inhibitCookie,
                                DBUS_TYPE_INVALID)) {
    this->ctx->logging.LogError(
        TAG, "Failed to append arguments to Screen Saver UnInhibit message");
    dbus_message_unref(msg);
    return 1;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      this->ctx->dbus.ssnConn, msg, -1, &this->ctx->dbus.ssnErr);
  if (!reply && dbus_error_is_set(&this->ctx->dbus.ssnErr)) {
    std::string errMsg = "D-Bus Error on Screen Saver UnInhibit: ";
    errMsg += this->ctx->dbus.ssnErr.message;
    this->ctx->logging.LogError(TAG, errMsg.c_str());
    dbus_error_free(&this->ctx->dbus.ssnErr);
    dbus_message_unref(msg);
    return 1;
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);
  inhibitCookie = -1;
  return 0;
}
