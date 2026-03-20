#pragma once

#include "services/header/context.hpp"
#include <cstdint>
#include <string>
#include <string_view>
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

class MprisManager {
  AppContext *ctx;
  std::vector<std::string> players;
  PlayerTrack playingTrack;

  void addPlayer(const std::string &playerName);
  void removePlayer(const std::string &playerName);

  bool PlayPause(MprisPlayPauseRequest req);
  bool PlayPauseDbusCall(const char *player);

  void GetPlayerInfo(MprisGetPlayerInfoRequest req);
  int GetPlayerInfoDbusCall(const char *player, PlayerTrack *track);

  bool GetPosition(MprisGetPositionRequest req);
  int GetCurrentPositionDbusCall();

  int SetPosition(MprisSetPositionRequest req);
  int PreviousTrack(MprisPreviousTrackRequest req);
  int NextTrack(MprisNextTrackRequest req);

public:
  MprisManager(AppContext *appCtx);

  bool hasPlayer() const;
  PlayerTrack getPlayingTrack() const;
  
  void handlePlayerChangesDbus(const std::string_view name, const std::string_view newOwner);
};
