#pragma once

#include "services/header/comm_types.hpp"
#include "services/header/context.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

struct PlayerTrack {
  std::string playerName;
  std::string trackId;
  std::string title;
  uint64_t currPos, length;
};

struct MprisPlayPauseRequest {
  uint64_t correlationId;
};

struct MprisGetPlayerInfoRequest {
  uint64_t correlationId;
};

struct MprisGetPositionRequest {
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
    std::variant<MprisPlayPauseRequest, MprisGetPlayerInfoRequest,
                 MprisGetPositionRequest, MprisSetPositionRequest, MprisGetPlayerInfoRequest,
                 MprisNextTrackRequest, MprisPreviousTrackRequest>;

class MprisManager {
  AppContext *ctx;
  std::vector<std::string> players;
  PlayerTrack playingTrack;

  void addPlayer(const std::string &playerName);
  void removePlayer(const std::string &playerName);

  ResponseMessage PlayPause(const MprisPlayPauseRequest& req);
  bool PlayPauseDbusCall(const char *player);

  ResponseMessage GetPlayerInfo(const MprisGetPlayerInfoRequest& req);
  int GetPlayerInfoDbusCall(const char *player, PlayerTrack *track);

  ResponseMessage GetPosition(const MprisGetPositionRequest& req);
  int GetCurrentPositionDbusCall();

  ResponseMessage SetPosition(const MprisSetPositionRequest& req);
  ResponseMessage PreviousTrack(const MprisPreviousTrackRequest& req);
  ResponseMessage NextTrack(const MprisNextTrackRequest& req);

public:
  MprisManager(AppContext *appCtx);

  bool hasPlayer() const;
  PlayerTrack getPlayingTrack() const;

  void handlePlayerChangesDbus(const std::string_view name,
                               const std::string_view newOwner);
  
  ResponseMessage handle(const MprisRequest& msg);
};
