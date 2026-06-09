#pragma once

#include "services/header/comm_types.hpp"
#include "services/header/context.hpp"
#include <fstream>
#include <sdbus-c++/IProxy.h>
#include <variant>

struct BrtSetLevelRequest {
  short brightness;

  ModuleType moduleType;
  uint64_t correlationId;
};

using BrtRequest = std::variant<BrtSetLevelRequest>;
class BrightnessManager {
  AppContext *ctx;
  std::unique_ptr<sdbus::IProxy> dbusProxy;
  std::ifstream blFile; // Backlight file
  std::string subsystem;

  short currentLvl;
  bool err;

  ResponseMessage setLvl(const BrtSetLevelRequest &req);

public:
  BrightnessManager(AppContext *ctx);

  void update(); // Only for Main Manager Thread Use

  short getLvl() const;

  ResponseMessage handle(const BrtRequest &req);
  ~BrightnessManager();
};
