#pragma once
#include "../../app/context.hpp"
#include <thread>

class NotificationManager {
  AppContext *ctx;
  std::thread notifThread;

  void captureNotification();

public:
  NotificationManager(AppContext *ctx);
  void RunService();

  // Notification Daemon Responses to Messages
  void handleNotifyCall(DBusMessage *msg);
  void handleGetCapabilitiesCall(DBusMessage *msg);
  void handleGetServerInformationCall(DBusMessage *msg);
  void handleCloseNotificationCall(DBusMessage *msg);
};
