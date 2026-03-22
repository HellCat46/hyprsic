#pragma once

#include "services/header/comm_types.hpp"
#include "services/header/context.hpp"
#include <variant>

struct ScrnSvrActivateRequest {
  uint64_t correlationId;
};

struct ScrnSvrDeActivateRequest {
  uint64_t correlationId;
};

using ScrnSvrRequest = std::variant<ScrnSvrActivateRequest, ScrnSvrDeActivateRequest>;
class ScreenSaverManager {
  AppContext *ctx;
  long int inhibitCookie = -1;

  ResponseMessage activateScreenSaver(ScrnSvrActivateRequest req);
  ResponseMessage deactivateScreenSaver(ScrnSvrDeActivateRequest req);

public:
  ScreenSaverManager(AppContext *ctx);
  
  
  ResponseMessage handle(const ScrnSvrRequest& msg);
};
