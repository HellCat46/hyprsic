#include "manager.hpp"
#include "dbus/dbus-protocol.h"
#include "dbus/dbus.h"
#include <cstring>
#include <string>

#define TAG "MprisManager"

MprisManager::MprisManager(AppContext *appCtx) : ctx(appCtx){
    
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
        //ctx->logging.LogInfo(TAG, "Found DBus Name: " + std::string(value));

        if (std::strncmp(value + 22, "playerctld", 10) == 0)
          continue;

        players.push_back(std::string(value));
      }
    }

    dbus_message_iter_next(&subIter);
  }
}

int MprisManager::PlayPause() {
  if (playingTrack.playerName.empty()) {
    ctx->logging.LogError(TAG,
                          "No player available to send PlayPause command.");
    return 1;
  }

  DBusMessage *msg = dbus_message_new_method_call(
      playingTrack.playerName.c_str(), "/org/mpris/MediaPlayer2",
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

int MprisManager::GetPlayerInfoDbusCall(const char *player, PlayerTrack *track) {
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

      dbus_message_iter_next(&dictEntryIter);
      if (std::strcmp(key, "xesam:title") == 0) {
        ctx->dbus.DictToString(&dictEntryIter, &track->title);
        track->playerName = std::string(player);
      } else if (std::strcmp(key, "mpris:length") == 0) {
        ctx->dbus.DictToInt64(&dictEntryIter, &track->length);
      } else if (std::strcmp(key, "mpris:trackid") == 0) {
        ctx->dbus.DictToString(&dictEntryIter, &track->trackId);
      }
    }
    dbus_message_iter_next(&arrayIter);
  }

  dbus_message_unref(msg);
  dbus_message_unref(reply);

  return 0;
}

int MprisManager::GetCurrentPositionDbusCall() {
  if (playingTrack.title.empty()) {
    ctx->logging.LogError(TAG, "No player available to get current position.");
    return 1;
  }

  DBusMessage *msg = dbus_message_new_method_call(
      playingTrack.playerName.c_str(), "/org/mpris/MediaPlayer2",
      "org.freedesktop.DBus.Properties", "Get");

  const char *interfaceName = "org.mpris.MediaPlayer2.Player";
  const char *propertyName = "Position";

  dbus_message_append_args(msg, DBUS_TYPE_STRING, &interfaceName,
                           DBUS_TYPE_STRING, &propertyName, DBUS_TYPE_INVALID);

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.ssnConn, msg, -1, &ctx->dbus.ssnErr);

  if (!reply && dbus_error_is_set(&ctx->dbus.ssnErr)) {
    std::string errMsg = "Failed to get a reply for Get Position. ";
    errMsg += ctx->dbus.ssnErr.message;
    ctx->logging.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.ssnErr);
    dbus_message_unref(msg);
    return 1;
  }

  DBusMessageIter iter;
  dbus_message_iter_init(reply, &iter);

  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT) {
    ctx->logging.LogError(TAG, "Invalid response type for Get Position");
    dbus_message_unref(reply);
    dbus_message_unref(msg);
    return 1;
  }

  DBusMessageIter variantIter;
  dbus_message_iter_recurse(&iter, &variantIter);

  if (dbus_message_iter_get_arg_type(&variantIter) != DBUS_TYPE_INT64) {
    ctx->logging.LogError(TAG, "Invalid variant type for Position");
    dbus_message_unref(reply);
    dbus_message_unref(msg);
    return 1;
  }

  int64_t position;
  dbus_message_iter_get_basic(&variantIter, &position);
  playingTrack.currPos = static_cast<uint64_t>(position);

  dbus_message_unref(msg);
  dbus_message_unref(reply);

  return 0;
}

int MprisManager::GetPlayerInfo() {
  bool titleFound = false;
  for (const auto &player : players) {

    PlayerTrack track;
    int res = GetPlayerInfoDbusCall(player.c_str(), &track);

    if (res == 0 && !track.title.empty()) {
      playingTrack = track;
      titleFound = true;
      break;
    }
  }
  
  
  if (titleFound) {
    GetCurrentPositionDbusCall();
    playingTrack.currPos /= 1000000; 
    playingTrack.length /= 1000000;
    return 0;
  }
  
  return 1;
}

int MprisManager::SetPosition(uint64_t position) {
  if (playingTrack.playerName.empty() || playingTrack.trackId.empty()) {
    ctx->logging.LogError(TAG,
                          "No player available to set position.");
    return 1;
  }

  DBusMessage *msg = dbus_message_new_method_call(
      playingTrack.playerName.c_str(), "/org/mpris/MediaPlayer2",
      "org.mpris.MediaPlayer2.Player", "SetPosition");

  const char *trackId = playingTrack.trackId.c_str();
  int64_t pos = static_cast<int64_t>(position * 1000000); 

  dbus_message_append_args(msg, DBUS_TYPE_OBJECT_PATH, &trackId,
                           DBUS_TYPE_INT64, &pos, DBUS_TYPE_INVALID);

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.ssnConn, msg, -1, &ctx->dbus.ssnErr);

  if (!reply && dbus_error_is_set(&ctx->dbus.ssnErr)) {
    std::string errMsg = "Failed to get a reply for SetPosition. ";
    errMsg += ctx->dbus.ssnErr.message;
    ctx->logging.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.ssnErr);
    dbus_message_unref(msg);
    return 1;
  }

  ctx->logging.LogInfo(TAG, "Set position to " + std::to_string(position) + "s.");

  dbus_message_unref(msg);
  dbus_message_ref(reply);

  return 0;
}

int MprisManager::PreviousTrack() {
  if (playingTrack.playerName.empty()) {
    ctx->logging.LogError(TAG,
                          "No player available to send PreviousTrack command.");
    return 1;
  }

  DBusMessage *msg = dbus_message_new_method_call(
      playingTrack.playerName.c_str(), "/org/mpris/MediaPlayer2",
      "org.mpris.MediaPlayer2.Player", "Previous");

  if (!msg) {
    ctx->logging.LogError(TAG,
                          "Failed to create PreviousTrack message for player.");
    return 1;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.ssnConn, msg, -1, &ctx->dbus.ssnErr);

  if (!reply && dbus_error_is_set(&ctx->dbus.ssnErr)) {
    std::string errMsg = "Failed to get a reply for PreviousTrack. ";
    errMsg += ctx->dbus.ssnErr.message;
    ctx->logging.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.ssnErr);
    dbus_message_unref(msg);
    return 1;
  }

  ctx->logging.LogInfo(TAG, "Sent PreviousTrack command to player.");

  dbus_message_unref(msg);
  dbus_message_ref(reply);

  return 0;
}

int MprisManager::NextTrack() {
  if (playingTrack.playerName.empty()) {
    ctx->logging.LogError(TAG,
                          "No player available to send NextTrack command.");
    return 1;
  }

  DBusMessage *msg = dbus_message_new_method_call(
      playingTrack.playerName.c_str(), "/org/mpris/MediaPlayer2",
      "org.mpris.MediaPlayer2.Player", "Next");

  if (!msg) {
    ctx->logging.LogError(TAG,
                          "Failed to create NextTrack message for player.");
    return 1;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.ssnConn, msg, -1, &ctx->dbus.ssnErr);

  if (!reply && dbus_error_is_set(&ctx->dbus.ssnErr)) {
    std::string errMsg = "Failed to get a reply for NextTrack. ";
    errMsg += ctx->dbus.ssnErr.message;
    ctx->logging.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.ssnErr);
    dbus_message_unref(msg);
    return 1;
  }

  ctx->logging.LogInfo(TAG, "Sent NextTrack command to player.");

  dbus_message_unref(msg);
  dbus_message_ref(reply);

  return 0;
}