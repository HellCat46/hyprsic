#pragma once

#include "services/header/context.hpp"

class ScreenSaverManager {
  AppContext *ctx;
  long int inhibitCookie = -1;
  
  int activateScreenSaver();
  int deactivateScreenSaver();

public:
  ScreenSaverManager(AppContext *ctx);
};
