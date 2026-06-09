#pragma once
#include "services/header/context.hpp"
#include <cstdint>
#include <memory>
#include <sdbus-c++/IObject.h>
#include <sdbus-c++/Message.h>
#include <string>
#include <unordered_map>

struct ImageData {
    int32_t width;
    int32_t height;
    int32_t rowstride;
    bool    hasAlpha;
    int32_t bitsPerSample;
    int32_t channels;
    std::vector<uint8_t> pixels;
};

class NotificationManager {
  AppContext *ctx;
  std::unique_ptr<sdbus::IObject> dbusObject;
  uint32_t notifId;
  
  std::unordered_map<std::string, GtkWidget *> notifications;

  // Notification Daemon Responses to Messages
  bool handleNotifyCallDbus(sdbus::MethodCall& msg, Notification& notif);
  void handleGetCapabilitiesCallDbus(sdbus::MethodCall& msg);
  void handleGetServerInformationCallDbus(sdbus::MethodCall& msg);
  void handleCloseNotificationCallDbus(sdbus::MethodCall& msg);

public:
  bool dnd;
  NotificationManager(AppContext *ctx);
};
