#pragma once

#include "services/header/context.hpp"

class ScreenSaverManager {
  AppContext *ctx;

public:
  long int inhibitCookie = -1;
  ScreenSaverManager(AppContext *ctx);
  int activateScreenSaver();
  int deactivateScreenSaver();
};
