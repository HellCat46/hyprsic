#pragma once

#include "services/header/comm_types.hpp"
#include "services/header/context.hpp"
#include <cstdint>
#include <sdbus-c++/IProxy.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

struct PlayerTrack {
  std::string playerName;
  std::string trackId;
  std::string title;
  uint64_t currPos, length;
};

struct MprisPlayPauseRequest {
  uint64_t correlationId;
};

struct MprisPlayPauseResponse {
  uint64_t correlationId;
};

struct MprisGetPositionRequest {
  uint64_t correlationId;
};

struct MprisGetPositionResponse {
  uint64_t correlationId;
};


struct MprisSetPositionRequest {
  uint64_t position;

  uint64_t correlationId;
};

struct MprisPreviousTrackRequest {
  uint64_t correlationId;
};

struct MprisNextTrackRequest {
  uint64_t correlationId;
};

using MprisRequest =
    std::variant<MprisPlayPauseRequest, MprisGetPositionRequest,
                 MprisSetPositionRequest, MprisNextTrackRequest,
                 MprisPreviousTrackRequest>;

class MprisManager {
  AppContext *ctx;
  std::unordered_map<std::string, std::unique_ptr<sdbus::IProxy>> players;
  PlayerTrack playingTrack;

  void addPlayer(const std::string &playerName);
  void removePlayer(const std::string &playerName);
  
  void GetPlayerInfo();
  int GetPlayerInfoDbusCall(const std::string& player);

  ResponseMessage PlayPause(const MprisPlayPauseRequest &req);

  ResponseMessage GetPosition(const MprisGetPositionRequest &req);
  int GetCurrentPositionDbusCall();

  ResponseMessage SetPosition(const MprisSetPositionRequest &req);
  ResponseMessage PreviousTrack(const MprisPreviousTrackRequest &req);
  ResponseMessage NextTrack(const MprisNextTrackRequest &req);

public:
  MprisManager(AppContext *appCtx);
  void update();

  bool hasPlayer() const;
  PlayerTrack getPlayingTrack() const;

  void handlePlayerChangesDbus(const std::string &name,
                               const std::string_view newOwner);

  ResponseMessage handle(const MprisRequest &msg);
};
