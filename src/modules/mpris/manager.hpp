#pragma once

#include "../../app/context.hpp"
#include <string>
#include <utility>
#include <vector>

class MprisManager {
  std::vector<std::string> players;
  AppContext *ctx;

public:
  std::pair<std::string, std::string> showPlayerTitle;
  MprisManager(AppContext *appCtx);

  int PlayPause();
  int PlayPauseDbusCall(const char *player);

  int GetTitle();
  int GetTitleDbusCall(const char *player, char **title);
};
