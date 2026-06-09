#include "header/manager.hpp"
#include "services/header/comm_types.hpp"
#include <filesystem>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Types.h>
#include <string>
#include <type_traits>
#include <variant>

#define TAG "BrightnessManager"

BrightnessManager::BrightnessManager(AppContext *ctx)
    : ctx(ctx), currentLvl(-1), err(true) {
  for (const auto &entry :
       std::filesystem::directory_iterator("/sys/class/backlight")) {
    if (entry.is_directory()) {
      std::string path = entry.path().string();

      blFile.open(path + "/brightness", std::ios::in);
      if (blFile.is_open()) {
        err = false;
        subsystem = path.substr(path.rfind("/") + 1);
        ctx->logger.LogInfo(TAG, "Backlight file found at: " + path);
        update();
        return;
      }
    }
  }

  if (!err) {
    try {
      dbusProxy = sdbus::createProxy(
          *ctx->dbus.sysConn, sdbus::ServiceName{"org.freedesktop.login1"},
          sdbus::ObjectPath{"/org/freedesktop/login1/session/auto"});
    } catch (const sdbus::Error &e) {
      ctx->logger.LogError(TAG, "Failed to create DBus proxy: " +
                                    std::string(e.what()));
      err = true;
    }
  }
}

void BrightnessManager::update() {
  if (err) {
    currentLvl = -1;
    return;
  }

  char ch;
  short brightness = 0;
  while (blFile.get(ch)) {
    if (ch < 48 || ch > 57) {
      continue;
    }

    brightness = (brightness * 10) + (ch - 48);
  }
  blFile.clear();
  blFile.seekg(0);

  currentLvl = brightness;
}

ResponseMessage BrightnessManager::setLvl(const BrtSetLevelRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};
  if (err) {
    resp.errMsg = "Init Failed";
    return resp;
  }
  if (req.brightness < 0 || req.brightness > 100) {
    resp.errMsg = "Brightness Value outside the range (0-100)";
    return resp;
  }

  try{
      
  dbusProxy->callMethod(sdbus::MethodName{"SetBrightness"})
      .onInterface("org.freedesktop.login1.Session")
      .withArguments(std::string{"backlight"}, subsystem, req.brightness);

  currentLvl = req.brightness;
  ctx->logger.LogInfo(TAG, "Brightness set to " +
                               std::to_string(req.brightness) + "%");

    resp.success = true;
  } catch (const std::exception &e) {
    resp.errMsg = e.what();
    ctx->logger.LogError(TAG, resp.errMsg);
  }
  return resp;
}

short BrightnessManager::getLvl() const { return currentLvl; }

// Kinda Pointless rn but surely i will add more functions later on
ResponseMessage BrightnessManager::handle(const BrtRequest &req) {
  ResponseMessage resp;

  std::visit(
      [&](auto &reqMsg) {
        using T = std::decay_t<decltype(reqMsg)>;

        if constexpr (std::is_same_v<T, BrtSetLevelRequest>) {
          resp = setLvl(reqMsg);
        }
      },
      req);

  return resp;
}

BrightnessManager::~BrightnessManager() {
  if (blFile.is_open())
    blFile.close();
}
