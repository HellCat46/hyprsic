#include "manager.hpp"
#include "dbus/dbus-protocol.h"
#include "dbus/dbus.h"
#include <filesystem>

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

bool BrightnessManager::setLvl(short brightness) {
  if (err)
    return false;
  if (brightness < 0 || brightness > 100)
    return false;

  DBusMessage *msg = dbus_message_new_method_call(
      "org.freedesktop.login1", "/org/freedesktop/login1/session/auto",
      "org.freedesktop.login1.Session", "SetBrightness");
  if (!msg) {
    ctx->logger.LogError(TAG, "Failed to create DBus message");
    return false;
  }

  DBusMessageIter args;
  dbus_message_iter_init_append(msg, &args);

  const char *backlight = "backlight";
  dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &backlight);

  const char *subsystemStr = subsystem.c_str();
  dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &subsystemStr);
  dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &brightness);

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(
      ctx->dbus.sysConn, msg, 1000, nullptr);
  if (!reply) {
    ctx->logger.LogError(TAG, "Failed to send DBus message");
    dbus_message_unref(msg);
    return false;
  }

  currentLvl = brightness;
  ctx->logger.LogInfo(TAG,
                      "Brightness set to " + std::to_string(brightness) + "%");

  dbus_message_unref(msg);
  dbus_message_unref(reply);
  return true;
}

short BrightnessManager::getLvl() const { return currentLvl; }

BrightnessManager::~BrightnessManager() {
  if (blFile.is_open())
    blFile.close();
}
