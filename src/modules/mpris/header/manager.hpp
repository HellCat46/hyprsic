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

class MprisManager {
  AppContext *ctx;
  std::vector<std::string> players;
  PlayerTrack playingTrack;

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

public:
  MprisManager(AppContext *appCtx);

  bool hasPlayer() const;
  PlayerTrack getPlayingTrack() const;
  
  void handlePlayerChangesDbus(const std::string_view name, const std::string_view newOwner);
};
