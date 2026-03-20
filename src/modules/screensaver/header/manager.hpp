#pragma once

#include "services/header/context.hpp"

struct ScrnSvrActivateRequest {
  long int correlationId;
};

struct ScrnSvrDeActivateRequest {
  long int correlationId;
};

class ScreenSaverManager {
  AppContext *ctx;
  long int inhibitCookie = -1;

  int activateScreenSaver(ScrnSvrActivateRequest req);
  int deactivateScreenSaver(ScrnSvrDeActivateRequest req);

public:
  ScreenSaverManager(AppContext *ctx);
};
