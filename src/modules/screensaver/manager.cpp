#include "header/manager.hpp"
#include "dbus/dbus-protocol.h"
#include "dbus/dbus.h"
#include "services/header/comm_types.hpp"

#define TAG "ScreenSaverManager"

ScreenSaverManager::ScreenSaverManager(AppContext *ctx) : ctx(ctx) {}

ResponseMessage
ScreenSaverManager::activateScreenSaver(const ScrnSvrActivateRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  DBusMessage *msg = dbus_message_new_method_call(
      "org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver",
      "org.freedesktop.ScreenSaver", "Inhibit");
  if (!msg) {
    resp.errMsg = "Failed to create D-Bus message for Screen Saver Inhibit";
    this->ctx->logger.LogError(TAG, resp.errMsg);

    return resp;
  }

  const char *app_name = "Hyprsic";
  const char *reason = "Activating Screen Saver";

  if (!dbus_message_append_args(msg, DBUS_TYPE_STRING, &app_name,
                                DBUS_TYPE_STRING, &reason, DBUS_TYPE_INVALID)) {
    resp.errMsg = "Failed to append arguments to Screen Saver Inhibit message;";
    this->ctx->logger.LogError(TAG, resp.errMsg);
    dbus_message_unref(msg);

    return resp;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      this->ctx->dbus.ssnConn, msg, -1, &this->ctx->dbus.ssnErr);
  if (!reply && dbus_error_is_set(&this->ctx->dbus.ssnErr)) {
    resp.errMsg = "D-Bus Error on Screen Saver Inhibit: ";
    resp.errMsg += this->ctx->dbus.ssnErr.message;
    this->ctx->logger.LogError(TAG, resp.errMsg);

    dbus_error_free(&this->ctx->dbus.ssnErr);
    dbus_message_unref(msg);
    return resp;
  }

  DBusMessageIter iter;
  if (!dbus_message_iter_init(reply, &iter)) {
    resp.errMsg = "Screen Saver Inhibit reply has no arguments";
    this->ctx->logger.LogError(TAG, resp.errMsg);
    dbus_message_unref(msg);
    dbus_message_unref(reply);

    return resp;
  }
  dbus_message_iter_get_basic(&iter, &inhibitCookie);

  dbus_message_unref(msg);
  dbus_message_unref(reply);

  resp.success = true;
  return resp;
}

ResponseMessage
ScreenSaverManager::deactivateScreenSaver(const ScrnSvrDeActivateRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  DBusMessage *msg = dbus_message_new_method_call(
      "org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver",
      "org.freedesktop.ScreenSaver", "UnInhibit");
  if (!msg) {
    resp.errMsg = "Failed to create D-Bus message for Screen Saver UnInhibit";

    this->ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  if (!dbus_message_append_args(msg, DBUS_TYPE_UINT32, &inhibitCookie,
                                DBUS_TYPE_INVALID)) {
    resp.errMsg =
        "Failed to append arguments to Screen Saver UnInhibit message";
    this->ctx->logger.LogError(TAG, resp.errMsg);
    dbus_message_unref(msg);

    return resp;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      this->ctx->dbus.ssnConn, msg, -1, &this->ctx->dbus.ssnErr);
  if (!reply && dbus_error_is_set(&this->ctx->dbus.ssnErr)) {
    resp.errMsg = "D-Bus Error on Screen Saver UnInhibit: ";
    resp.errMsg += this->ctx->dbus.ssnErr.message;

    this->ctx->logger.LogError(TAG, resp.errMsg);
    dbus_error_free(&this->ctx->dbus.ssnErr);
    dbus_message_unref(msg);

    return resp;
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);
  inhibitCookie = -1;

  resp.success = true;
  return resp;
}

bool ScreenSaverManager::isActive() { return inhibitCookie != -1; }

ResponseMessage ScreenSaverManager::handle(const ScrnSvrRequest &req) {
  ResponseMessage resp;

  std::visit(
      [&](auto &reqMsg) {
        using T = std::decay_t<decltype(reqMsg)>;

        if constexpr (std::is_same_v<T, ScrnSvrActivateRequest>) {
          resp = activateScreenSaver(reqMsg);
        } else if constexpr (std::is_same_v<T, ScrnSvrDeActivateRequest>) {
          resp = deactivateScreenSaver(reqMsg);
        }
      },
      req);

  return resp;
}
