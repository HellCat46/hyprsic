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
  long int correlationId;
};

struct MprisGetPlayerInfoRequest {
  long int correlationId;
};

struct MprisGetPositionRequest {
  long int correlationId;
};

struct MprisSetPositionRequest {
  uint64_t position;

  long int correlationId;
};

struct MprisPreviousTrackRequest {
  long int correlationId;
};

struct MprisNextTrackRequest {
  long int correlationId;
};

using MprisRequest =
    std::variant<MprisPlayPauseRequest, MprisGetPlayerInfoRequest,
                 MprisGetPositionRequest, MprisGetPlayerInfoRequest,
                 MprisNextTrackRequest, MprisPreviousTrackRequest>;

class MprisManager {
  AppContext *ctx;
  std::vector<std::string> players;
  PlayerTrack playingTrack;

  void addPlayer(const std::string &playerName);
  void removePlayer(const std::string &playerName);

  ResponseMessage PlayPause(MprisPlayPauseRequest req);
  bool PlayPauseDbusCall(const char *player);

  ResponseMessage GetPlayerInfo(MprisGetPlayerInfoRequest req);
  int GetPlayerInfoDbusCall(const char *player, PlayerTrack *track);

  ResponseMessage GetPosition(MprisGetPositionRequest req);
  int GetCurrentPositionDbusCall();

  ResponseMessage SetPosition(MprisSetPositionRequest req);
  ResponseMessage PreviousTrack(MprisPreviousTrackRequest req);
  ResponseMessage NextTrack(MprisNextTrackRequest req);

public:
  MprisManager(AppContext *appCtx);

  bool hasPlayer() const;
  PlayerTrack getPlayingTrack() const;

  void handlePlayerChangesDbus(const std::string_view name,
                               const std::string_view newOwner);
  
  ResponseMessage handle(const MprisRequest& msg);
};
