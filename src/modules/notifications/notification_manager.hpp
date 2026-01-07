#pragma once
#include "../../app/context.hpp"

class NotificationManager {
    AppContext* ctx;
  public:
    NotificationManager(AppContext* ctx);
    void captureNotification(); 
    
    // Notification Daemon Responses to Messages
    void handleNotifyCall(DBusMessage* msg);
    void handleGetCapabilitiesCall(DBusMessage* msg);
    void handleGetServerInformationCall(DBusMessage* msg);
    void handleCloseNotificationCall(DBusMessage* msg);
};