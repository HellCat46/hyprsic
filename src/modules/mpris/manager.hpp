#pragma once

#include "../../app/context.hpp"
#include <cstdint>
#include <string>
#include <vector>

struct PlayerTrack {
  std::string playerName;
  std::string trackId;
  std::string title;
  uint64_t currPos, length;
};

class MprisManager {
  std::vector<std::string> players;
  AppContext *ctx;

public:
  bool hasPlayer = false;
  PlayerTrack playingTrack;
  MprisManager(AppContext *appCtx);
  void addPlayer(const std::string &playerName);
  void removePlayer(const std::string &playerName);

  bool PlayPause();
  bool PlayPauseDbusCall(const char *player);

  void GetPlayerInfo();
  int GetPlayerInfoDbusCall(const char *player, PlayerTrack *track);
  
  bool GetPosition();
  int GetCurrentPositionDbusCall();

  int SetPosition(uint64_t position);
  int PreviousTrack();
  int NextTrack();
};
