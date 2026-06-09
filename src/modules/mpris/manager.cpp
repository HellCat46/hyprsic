#include "header/manager.hpp"
#include "services/header/comm_types.hpp"
#include <cstdint>
#include <map>
#include <sdbus-c++/Error.h>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/Types.h>
#include <string>

#define TAG "MprisManager"

MprisManager::MprisManager(AppContext *appCtx) : ctx(appCtx) {

  try {
    std::unique_ptr<sdbus::IProxy> proxy = sdbus::createProxy(
        *ctx->dbus.ssnConn, sdbus::ServiceName{"org.freedesktop.DBus"},
        sdbus::ObjectPath{"/org/freedesktop/DBus"});

    sdbus::MethodReply reply = proxy->callMethod(
        proxy->createMethodCall(sdbus::InterfaceName{"org.freedesktop.DBus"},
                                sdbus::MethodName{"ListNames"}));

    std::vector<std::string> playerList;
    reply >> playerList;
    for (const auto &player : playerList) {
      if (player.find("mpris") != std::string::npos) {
        addPlayer(player);
      }
    }
  } catch (const std::exception &e) {
    ctx->logger.LogError(TAG,
                         "Failed to list DBus names: " + std::string(e.what()));
  }

  update();
}

void MprisManager::update() { GetPlayerInfo(); }

ResponseMessage MprisManager::PlayPause(const MprisPlayPauseRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  if (playingTrack.playerName.empty()) {
    resp.errMsg = "No player available to send PlayPause command.";
    ctx->logger.LogError(TAG, resp.errMsg);

    return resp;
  }

  auto it = players.find(playingTrack.playerName);
  if (it == players.end()) {
    resp.errMsg = "Player not found.";
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  try {
    it->second->callMethod(sdbus::MethodName{"PlayPause"})
        .onInterface(sdbus::InterfaceName{"org.mpris.MediaPlayer2.Player"});

    ctx->logger.LogInfo(TAG, "Sent PlayPause command to player.");

    resp.success = true;
  } catch (const sdbus::Error &e) {
    resp.errMsg = "Failed to send PlayPause command: " + std::string{e.what()};
    ctx->logger.LogError(TAG, resp.errMsg);
  }
  return resp;
}

int MprisManager::GetPlayerInfoDbusCall(const std::string &player,
                                        PlayerTrack *track) {

  auto it = players.find(player);
  if (it == players.end()) {
    return 1;
  }

  try {
    sdbus::MethodCall msg = it->second->createMethodCall(
        sdbus::InterfaceName{"org.freedesktop.DBus.Properties"},
        sdbus::MethodName{"Get"});

    msg << std::string{"org.mpris.MediaPlayer2.Player"}
        << std::string{"Metadata"};

    sdbus::MethodReply reply = it->second->callMethod(msg);

    sdbus::Variant variant;
    reply >> variant;

    auto metadata = variant.get<std::map<std::string, sdbus::Variant>>();

    if (metadata.contains("xesam:title")) {
      track->title = metadata.at("xesam:title").get<std::string>();
      track->playerName = std::string(player);
    }

    if (metadata.contains("mpris:length")) {
      track->length = metadata.at("mpris:length").get<int64_t>() / 1000000;
    }

    if (metadata.contains("mpris:trackid")) {
      track->trackId = metadata.at("mpris:trackid").get<sdbus::ObjectPath>();
    }

  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG,
                         "Failed to get player info: " + std::string{e.what()});
    return 1;
  }

  return 0;
}

int MprisManager::GetCurrentPositionDbusCall() {
  if (playingTrack.title.empty()) {
    ctx->logger.LogError(TAG, "No player available to get current position.");
    return 1;
  }

  auto it = players.find(playingTrack.playerName);
  if (it == players.end()) {
    ctx->logger.LogError(TAG, "No player available to get current position.");
    return 1;
  }

  try {
    sdbus::MethodCall msg = it->second->createMethodCall(
        sdbus::InterfaceName{"org.freedesktop.DBus.Properties"},
        sdbus::MethodName{"Get"});
    msg << std::string{"org.mpris.MediaPlayer2.Player"}
        << std::string{"Position"};

    sdbus::MethodReply reply = it->second->callMethod(msg);
    sdbus::Variant variant;
    reply >> variant;

    playingTrack.currPos = variant.get<int64_t>();

  } catch (const sdbus::Error &e) {
    ctx->logger.LogError(TAG, "Failed to get current position: " +
                                  std::string{e.what()});
    return 1;
  }

  return 0;
}

void MprisManager::GetPlayerInfo() {

  for (const auto &[playerName, playerProxy] : players) {

    PlayerTrack track;
    int res = GetPlayerInfoDbusCall(playerName, &track);

    if (res == 0 && !track.title.empty()) {
      playingTrack = track;
      return;
    }
  }

  playingTrack.trackId = "";
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

  auto it = players.find(playingTrack.playerName);
  if (it == players.end()) {
    resp.errMsg = "Player not found.";
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  try {
    it->second->callMethod(sdbus::MethodName{"SetPosition"})
        .onInterface(sdbus::InterfaceName{"org.mpris.MediaPlayer2.Player"})
        .withArguments(sdbus::ObjectPath{playingTrack.trackId},
                       int64_t{static_cast<int64_t>(req.position * 1000000)});
  } catch (const sdbus::Error &e) {
    resp.errMsg = e.what();
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  ctx->logger.LogInfo(TAG,
                      "Set position to " + std::to_string(req.position) + "s.");
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

  auto it = players.find(playingTrack.playerName);
  if (it == players.end()) {
    resp.errMsg = "Player not found.";
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  try {
    it->second->callMethod(sdbus::MethodName{"Previous"})
        .onInterface(sdbus::InterfaceName{"org.mpris.MediaPlayer2.Player"});
  } catch (const sdbus::Error &e) {
    resp.errMsg = e.what();
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  ctx->logger.LogInfo(TAG, "Sent PreviousTrack command to player.");
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

  auto it = players.find(playingTrack.playerName);
  if (it == players.end()) {
    resp.errMsg = "Player not found.";
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  try {
    it->second->callMethod(sdbus::MethodName{"Next"})
        .onInterface(sdbus::InterfaceName{"org.mpris.MediaPlayer2.Player"});
  } catch (const sdbus::Error &e) {
    resp.errMsg = e.what();
    ctx->logger.LogError(TAG, resp.errMsg);

    return resp;
  }

  ctx->logger.LogInfo(TAG, "Sent NextTrack command to player.");

  resp.success = true;
  return resp;
}

void MprisManager::addPlayer(const std::string &playerName) {
  players.insert(
      {playerName,
       sdbus::createProxy(*ctx->dbus.ssnConn, sdbus::ServiceName{playerName},
                          sdbus::ObjectPath{"/org/mpris/MediaPlayer2"})});
  ctx->logger.LogInfo(TAG, "Added new MPRIS Player: " + playerName);
}

void MprisManager::removePlayer(const std::string &playerName) {
  auto it = players.find(playerName);
  if (it == players.end())
    return;

  players.erase(it);
  ctx->logger.LogInfo(TAG, "Removed MPRIS Player: " + playerName);
}

bool MprisManager::hasPlayer() const { return !playingTrack.trackId.empty(); }

PlayerTrack MprisManager::getPlayingTrack() const { return playingTrack; }

void MprisManager::handlePlayerChangesDbus(const std::string &name,
                                           const std::string_view newOwner) {
  if (name.find("org.mpris.MediaPlayer2") != std::string::npos) {

    if (newOwner.length() != 0) {
      addPlayer(name);
    } else {
      removePlayer(name);
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
        } else if constexpr (std::is_same_v<T, MprisSetPositionRequest>) {
          resp = SetPosition(reqMsg);
        } else if constexpr (std::is_same_v<T, MprisGetPositionRequest>) {
          resp = GetPosition(reqMsg);
        } else if constexpr (std::is_same_v<T, MprisNextTrackRequest>) {
          resp = NextTrack(reqMsg);
        } else if constexpr (std::is_same_v<T, MprisPreviousTrackRequest>) {
          resp = PreviousTrack(reqMsg);
        }
      },
      req);

  return resp;
}
