#include "manager.hpp"
#include "dbus/dbus-protocol.h"
#include "dbus/dbus.h"
#include <cstring>
#include <string>

#define TAG "MprisManager"

MprisManager::MprisManager(AppContext *appCtx) {
  ctx = appCtx;

  DBusMessage *msg = dbus_message_new_method_call(
      "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
      "ListNames");

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.ssnConn, msg, -1, &ctx->dbus.ssnErr);

  dbus_message_unref(msg);

  DBusMessageIter iter, subIter;
  dbus_message_iter_init(reply, &iter);

  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
    ctx->logging.LogError(TAG, "Invalid response type for ListNames");
    dbus_message_unref(reply);
    return;
  }

  dbus_message_iter_recurse(&iter, &subIter);
  int type;

  while ((type = dbus_message_iter_get_arg_type(&subIter)) !=
         DBUS_MESSAGE_TYPE_INVALID) {

    if (type == DBUS_TYPE_STRING) {
      char *value;
      dbus_message_iter_get_basic(&subIter, &value);

      if (std::strncmp(value, "org.mpris.MediaPlayer2", 22) == 0) {
        ctx->logging.LogInfo(TAG, "Found DBus Name: " + std::string(value));

        if (std::strncmp(value + 22, "playerctld", 10) == 0)
          continue;

        players.push_back(std::string(value));
      }
    }

    dbus_message_iter_next(&subIter);
  }
}

int MprisManager::PlayPause() {
  if (showPlayerTitle.first.empty()) {
    ctx->logging.LogError(TAG,
                          "No player available to send PlayPause command.");
    return 1;
  }

  DBusMessage *msg = dbus_message_new_method_call(
      showPlayerTitle.first.c_str(), "/org/mpris/MediaPlayer2",
      "org.mpris.MediaPlayer2.Player", "PlayPause");

  if (!msg) {
    ctx->logging.LogError(TAG,
                          "Failed to create PlayPause message for player.");
    return 1;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.ssnConn, msg, -1, &ctx->dbus.ssnErr);

  if (!reply && dbus_error_is_set(&ctx->dbus.ssnErr)) {
    std::string errMsg = "Failed to get a reply for PlayPause. ";
    errMsg += ctx->dbus.ssnErr.message;
    ctx->logging.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.ssnErr);
    dbus_message_unref(msg);
    return 1;
  }

  ctx->logging.LogInfo(TAG, "Sent PlayPause command to player.");

  dbus_message_unref(msg);
  dbus_message_ref(reply);

  return 0;
}

int MprisManager::GetTitleDbusCall(const char *player, char **title) {
  DBusMessage *msg =
      dbus_message_new_method_call(player, "/org/mpris/MediaPlayer2",
                                   "org.freedesktop.DBus.Properties", "Get");

  const char *interfaceName = "org.mpris.MediaPlayer2.Player";
  const char *propertyName = "Metadata";

  dbus_message_append_args(msg, DBUS_TYPE_STRING, &interfaceName,
                           DBUS_TYPE_STRING, &propertyName, DBUS_TYPE_INVALID);

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.ssnConn, msg, -1, &ctx->dbus.ssnErr);

  if (!reply && dbus_error_is_set(&ctx->dbus.ssnErr)) {
    std::string errMsg = "Failed to get a reply for Get Metadata. ";
    errMsg += ctx->dbus.ssnErr.message;
    ctx->logging.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.ssnErr);
    dbus_message_unref(msg);
    return 1;
  }

  DBusMessageIter iter;
  dbus_message_iter_init(reply, &iter);

  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT) {
    ctx->logging.LogError(TAG, "Invalid response type for Get Metadata");
    dbus_message_unref(reply);
    dbus_message_unref(msg);
    return 1;
  }

  DBusMessageIter variantIter;
  dbus_message_iter_recurse(&iter, &variantIter);

  if (dbus_message_iter_get_arg_type(&variantIter) != DBUS_TYPE_ARRAY) {
    ctx->logging.LogError(TAG, "Invalid variant type for Metadata");
    dbus_message_unref(reply);
    dbus_message_unref(msg);
    return 1;
  }

  DBusMessageIter arrayIter;
  dbus_message_iter_recurse(&variantIter, &arrayIter);

  int type;
  while ((type = dbus_message_iter_get_arg_type(&arrayIter)) !=
         DBUS_MESSAGE_TYPE_INVALID) {
    if (type == DBUS_TYPE_DICT_ENTRY) {
      DBusMessageIter dictEntryIter;
      dbus_message_iter_recurse(&arrayIter, &dictEntryIter);

      char *key;
      dbus_message_iter_get_basic(&dictEntryIter, &key);

      if (std::strcmp(key, "xesam:title") == 0) {
        dbus_message_iter_next(&dictEntryIter);
        DBusMessageIter valueIter;
        dbus_message_iter_recurse(&dictEntryIter, &valueIter);

        if (dbus_message_iter_get_arg_type(&valueIter) == DBUS_TYPE_STRING) {
          dbus_message_iter_get_basic(&valueIter, title);
        }
      }
    }
    dbus_message_iter_next(&arrayIter);
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);

  return 0;
}

int MprisManager::GetTitle() {
  for (const auto &player : players) {
    char *title = nullptr;
    int res = GetTitleDbusCall(player.c_str(), &title);

    if (res == 0 && title != nullptr) {
      showPlayerTitle = std::make_pair(player, title);
      return 0;
    }
  }
  return 1;
}
