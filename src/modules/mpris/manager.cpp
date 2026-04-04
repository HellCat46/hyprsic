#include "header/manager.hpp"
#include "dbus/dbus-protocol.h"
#include "dbus/dbus.h"
#include "services/header/comm_types.hpp"
#include "utils/helper_func.hpp"
#include <algorithm>
#include <cstring>
#include <string>

#define TAG "MprisManager"

MprisManager::MprisManager(AppContext *appCtx) : ctx(appCtx) {

  DBusMessage *msg = dbus_message_new_method_call(
      "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
      "ListNames");

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.ssnConn, msg, -1, &ctx->dbus.ssnErr);

  DBusMessageIter iter, subIter;
  dbus_message_iter_init(reply, &iter);

  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
    ctx->logger.LogError(TAG, "Invalid response type for ListNames");
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

      if (HelperFunc::saferStrNCmp(value, "org.mpris.MediaPlayer2", 22)) {
        // ctx->logger.LogInfo(TAG, "Found DBus Name: " + std::string(value));

        if (HelperFunc::saferStrNCmp(value + 22, "playerctld", 10))
          continue;

        players.push_back(std::string(value));
      }
    }

    dbus_message_iter_next(&subIter);
  }

  dbus_message_unref(reply);
  dbus_message_unref(msg);
}

ResponseMessage MprisManager::PlayPause(const MprisPlayPauseRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  if (playingTrack.playerName.empty()) {
    resp.errMsg = "No player available to send PlayPause command.";
    ctx->logger.LogError(TAG, resp.errMsg);

    return resp;
  }

  DBusMessage *msg = dbus_message_new_method_call(
      playingTrack.playerName.c_str(), "/org/mpris/MediaPlayer2",
      "org.mpris.MediaPlayer2.Player", "PlayPause");

  if (!msg) {
    resp.errMsg = "Failed to create PlayPause message for player.";
    ctx->logger.LogError(TAG, resp.errMsg);

    return resp;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.ssnConn, msg, -1, &ctx->dbus.ssnErr);

  if (!reply && dbus_error_is_set(&ctx->dbus.ssnErr)) {
    resp.errMsg = "Failed to get a reply for PlayPause. ";
    resp.errMsg += ctx->dbus.ssnErr.message;
    ctx->logger.LogError(TAG, resp.errMsg);
    dbus_error_free(&ctx->dbus.ssnErr);
    dbus_message_unref(msg);

    return resp;
  }

  ctx->logger.LogInfo(TAG, "Sent PlayPause command to player.");

  dbus_message_unref(msg);
  dbus_message_ref(reply);

  resp.success = true;
  return resp;
}

int MprisManager::GetPlayerInfoDbusCall(const char *player,
                                        PlayerTrack *track) {
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
    ctx->logger.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.ssnErr);
    dbus_message_unref(msg);
    return 1;
  }

  DBusMessageIter iter;
  dbus_message_iter_init(reply, &iter);

  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT) {
    ctx->logger.LogError(TAG, "Invalid response type for Get Metadata");
    dbus_message_unref(reply);
    dbus_message_unref(msg);
    return 1;
  }

  DBusMessageIter variantIter;
  dbus_message_iter_recurse(&iter, &variantIter);

  if (dbus_message_iter_get_arg_type(&variantIter) != DBUS_TYPE_ARRAY) {
    ctx->logger.LogError(TAG, "Invalid variant type for Metadata");
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
      if (HelperFunc::saferStrCmp(key, "xesam:title")) {
        ctx->dbus.DictToString(&dictEntryIter, track->title);
        track->playerName = std::string(player);
      } else if (HelperFunc::saferStrCmp(key, "mpris:length")) {
        ctx->dbus.DictToInt64(&dictEntryIter, track->length);
      } else if (HelperFunc::saferStrCmp(key, "mpris:trackid")) {
        ctx->dbus.DictToString(&dictEntryIter, track->trackId);
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
    ctx->logger.LogError(TAG, "No player available to get current position.");
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
    ctx->logger.LogError(TAG, errMsg);
    dbus_error_free(&ctx->dbus.ssnErr);
    dbus_message_unref(msg);
    return 1;
  }

  DBusMessageIter iter;
  dbus_message_iter_init(reply, &iter);

  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT) {
    ctx->logger.LogError(TAG, "Invalid response type for Get Position");
    dbus_message_unref(reply);
    dbus_message_unref(msg);
    return 1;
  }

  DBusMessageIter variantIter;
  dbus_message_iter_recurse(&iter, &variantIter);

  if (dbus_message_iter_get_arg_type(&variantIter) != DBUS_TYPE_INT64) {
    ctx->logger.LogError(TAG, "Invalid variant type for Position");
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

ResponseMessage
MprisManager::GetPlayerInfo(const MprisGetPlayerInfoRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  for (const auto &player : players) {

    PlayerTrack track;
    int res = GetPlayerInfoDbusCall(player.c_str(), &track);

    if (res == 0 && !track.title.empty()) {
      playingTrack = track;
      resp.success = true;
      return resp;
    }
  }

  playingTrack.trackId = "";
  resp.errMsg = "No Player Found";
  return resp;
}

ResponseMessage MprisManager::GetPosition(const MprisGetPositionRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  if (GetCurrentPositionDbusCall()) {
    resp.errMsg = "Failed to get Current Position from Dbus Interface";
    return resp;
  }

  playingTrack.currPos /= 1000000;
  playingTrack.length /= 1000000;

  resp.success = true;
  return resp;
}

ResponseMessage MprisManager::SetPosition(const MprisSetPositionRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  if (playingTrack.playerName.empty() || playingTrack.trackId.empty()) {
    resp.errMsg = "No player available to set position.";
    ctx->logger.LogError(TAG, resp.errMsg);

    return resp;
  }

  DBusMessage *msg = dbus_message_new_method_call(
      playingTrack.playerName.c_str(), "/org/mpris/MediaPlayer2",
      "org.mpris.MediaPlayer2.Player", "SetPosition");

  const char *trackId = playingTrack.trackId.c_str();
  int64_t pos = static_cast<int64_t>(req.position * 1000000);

  dbus_message_append_args(msg, DBUS_TYPE_OBJECT_PATH, &trackId,
                           DBUS_TYPE_INT64, &pos, DBUS_TYPE_INVALID);

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.ssnConn, msg, -1, &ctx->dbus.ssnErr);

  if (!reply && dbus_error_is_set(&ctx->dbus.ssnErr)) {
    resp.errMsg = "Failed to get a reply for SetPosition. ";
    resp.errMsg += ctx->dbus.ssnErr.message;
    ctx->logger.LogError(TAG, resp.errMsg);
    dbus_error_free(&ctx->dbus.ssnErr);
    dbus_message_unref(msg);

    return resp;
  }

  ctx->logger.LogInfo(TAG,
                      "Set position to " + std::to_string(req.position) + "s.");

  dbus_message_unref(msg);
  dbus_message_ref(reply);

  resp.success = true;
  return resp;
}

ResponseMessage
MprisManager::PreviousTrack(const MprisPreviousTrackRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  if (playingTrack.playerName.empty()) {
    resp.errMsg = "No player available to send PreviousTrack command.";
    ctx->logger.LogError(TAG, resp.errMsg);

    return resp;
  }

  DBusMessage *msg = dbus_message_new_method_call(
      playingTrack.playerName.c_str(), "/org/mpris/MediaPlayer2",
      "org.mpris.MediaPlayer2.Player", "Previous");

  if (!msg) {
    resp.errMsg = "Failed to create PreviousTrack message for player.";
    ctx->logger.LogError(TAG, resp.errMsg);

    return resp;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.ssnConn, msg, -1, &ctx->dbus.ssnErr);

  if (!reply && dbus_error_is_set(&ctx->dbus.ssnErr)) {
    resp.errMsg = "Failed to get a reply for PreviousTrack. ";
    resp.errMsg += ctx->dbus.ssnErr.message;

    ctx->logger.LogError(TAG, resp.errMsg);
    dbus_error_free(&ctx->dbus.ssnErr);
    dbus_message_unref(msg);

    return resp;
  }

  ctx->logger.LogInfo(TAG, "Sent PreviousTrack command to player.");

  dbus_message_unref(msg);
  dbus_message_ref(reply);

  resp.success = true;
  return resp;
}

ResponseMessage MprisManager::NextTrack(const MprisNextTrackRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  if (playingTrack.playerName.empty()) {
    resp.errMsg = "No player available to send NextTrack command.";
    ctx->logger.LogError(TAG, resp.errMsg);

    return resp;
  }

  DBusMessage *msg = dbus_message_new_method_call(
      playingTrack.playerName.c_str(), "/org/mpris/MediaPlayer2",
      "org.mpris.MediaPlayer2.Player", "Next");

  if (!msg) {
    resp.errMsg = "Failed to create NextTrack message for player.";
    ctx->logger.LogError(TAG, resp.errMsg);

    return resp;
  }

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.ssnConn, msg, -1, &ctx->dbus.ssnErr);

  if (!reply && dbus_error_is_set(&ctx->dbus.ssnErr)) {
    resp.errMsg = "Failed to get a reply for NextTrack. ";
    resp.errMsg += ctx->dbus.ssnErr.message;

    ctx->logger.LogError(TAG, resp.errMsg);
    dbus_error_free(&ctx->dbus.ssnErr);
    dbus_message_unref(msg);

    return resp;
  }

  ctx->logger.LogInfo(TAG, "Sent NextTrack command to player.");

  dbus_message_unref(msg);
  dbus_message_ref(reply);

  resp.success = true;
  return resp;
}

void MprisManager::addPlayer(const std::string &playerName) {
  players.push_back(playerName);
  ctx->logger.LogInfo(TAG, "Added new MPRIS Player: " + playerName);
}

void MprisManager::removePlayer(const std::string &playerName) {
  auto it = std::find(players.begin(), players.end(), playerName);
  if (it == players.end())
    return;

  players.erase(it);
  ctx->logger.LogInfo(TAG, "Removed MPRIS Player: " + playerName);
}

bool MprisManager::hasPlayer() const { return !playingTrack.trackId.empty(); }

PlayerTrack MprisManager::getPlayingTrack() const { return playingTrack; }

void MprisManager::handlePlayerChangesDbus(const std::string_view name,
                                           const std::string_view newOwner) {
  if (HelperFunc::saferStrNCmp(name.data(), "org.mpris.MediaPlayer2", 22)) {

    if (std::strlen(newOwner.data()) != 0) {
      addPlayer(name.data());
    } else {
      removePlayer(name.data());
    }
  }
}

ResponseMessage MprisManager::handle(const MprisRequest &req) {
  ResponseMessage resp;

  std::visit(
      [&](auto &reqMsg) {
        using T = std::decay_t<decltype(reqMsg)>;

        if constexpr (std::is_same_v<T, MprisPlayPauseRequest>) {
          resp = PlayPause(reqMsg);
        } else if constexpr (std::is_same_v<T, MprisGetPlayerInfoRequest>) {
          resp = GetPlayerInfo(reqMsg);
        } else if constexpr (std::is_same_v<T, MprisSetPositionRequest>) {
          resp = SetPosition(reqMsg);
        } else if constexpr (std::is_same_v<T, MprisGetPositionRequest>) {
          resp = GetPosition(reqMsg);
        } else if constexpr (std::is_same_v<T, MprisGetPlayerInfoRequest>) {
          resp = GetPlayerInfo(reqMsg);
        } else if constexpr (std::is_same_v<T, MprisNextTrackRequest>) {
          resp = NextTrack(reqMsg);
        } else if constexpr (std::is_same_v<T, MprisPreviousTrackRequest>) {
          resp = PreviousTrack(reqMsg);
        }
      },
      req);

  return resp;
}
