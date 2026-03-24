#include "header/manager.hpp"
#include "dbus/dbus-protocol.h"
#include "dbus/dbus.h"
#include "services/header/comm_types.hpp"
#include <filesystem>
#include <type_traits>
#include <variant>

#define TAG "BrightnessManager"

BrightnessManager::BrightnessManager(AppContext *ctx) : ctx(ctx) {
  currentLvl = -1;

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
  err = true;
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

  DBusMessage *msg = dbus_message_new_method_call(
      "org.freedesktop.login1", "/org/freedesktop/login1/session/auto",
      "org.freedesktop.login1.Session", "SetBrightness");
  if (!msg) {
    resp.errMsg = "Failed to create DBus message.";
    ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  DBusMessageIter args;
  dbus_message_iter_init_append(msg, &args);

  const char *backlight = "backlight";
  dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &backlight);

  const char *subsystemStr = subsystem.c_str();
  dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &subsystemStr);
  dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &req.brightness);

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, msg, 1000, nullptr);
  if (!reply) {
      resp.errMsg = "Failed to send DBus message";
    ctx->logger.LogError(TAG, resp.errMsg);
    dbus_message_unref(msg);
    return resp;
  }

  currentLvl = req.brightness;
  ctx->logger.LogInfo(TAG,
                      "Brightness set to " + std::to_string(req.brightness) + "%");

  dbus_message_unref(msg);
  dbus_message_unref(reply);
  
  resp.success = true;
  return resp;
}

short BrightnessManager::getLvl() const { return currentLvl; }

// Kinda Pointless rn but surely i will add more functions later on
ResponseMessage BrightnessManager::handle(const BrtRequest &req) {
    ResponseMessage resp;
    
    std::visit([&](auto& reqMsg){
        using T = std::decay_t<decltype(reqMsg)>;
        
        if constexpr (std::is_same_v<T, BrtSetLevelRequest>) {
            resp = setLvl(reqMsg);
        }
    }, req);
    
    return resp;
}

BrightnessManager::~BrightnessManager() {
  if (blFile.is_open())
    blFile.close();
}
