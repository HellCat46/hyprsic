#pragma once

#include "services/header/comm_types.hpp"
#include "services/header/context.hpp"
#include <memory>
#include <sdbus-c++/IProxy.h>
#include <variant>

struct ScrnSvrActivateRequest {
  ModuleType moduleType;
  uint64_t correlationId;
};

struct ScrnSvrDeActivateRequest {
  ModuleType moduleType;
  uint64_t correlationId;
};

using ScrnSvrRequest =
    std::variant<ScrnSvrActivateRequest, ScrnSvrDeActivateRequest>;
class ScreenSaverManager {
  AppContext *ctx;
  std::unique_ptr<sdbus::IProxy> dbusProxy;
  long int inhibitCookie = -1;

  ResponseMessage activateScreenSaver(const ScrnSvrActivateRequest &req);
  ResponseMessage deactivateScreenSaver(const ScrnSvrDeActivateRequest &req);

public:
  ScreenSaverManager(AppContext *ctx);

  bool isActive();
  ResponseMessage handle(const ScrnSvrRequest &msg);
};
