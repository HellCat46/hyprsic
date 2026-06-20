#include "header/manager.hpp"
#include "glib.h"
#include "glibmm/refptr.h"
#include "gtkmm-4.0/gdkmm/pixbuf.h"
#include "services/header/context.hpp"
#include <cstdint>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IObject.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/VTableItems.h>
#include <string>
#include <vector>

#define TAG "NotificationManager"

NotificationManager::NotificationManager(AppContext *app_ctx)
    : ctx(app_ctx), notifId(1), dnd(false) {
  try {
    ctx->dbus.ssnConn->requestName(
        sdbus::ServiceName{"org.freedesktop.Notifications"});

    dbusObject = sdbus::createObject(
        *ctx->dbus.ssnConn, sdbus::ObjectPath{"/org/freedesktop/Notifications"});

  } catch (const sdbus::Error &e) {
    std::string errMsg =
        "Failed to request name or create object for Notifications: ";
    errMsg += e.what();
    ctx->logger.LogError(TAG, errMsg);
    return;
  }

  try {
    dbusObject
        ->addVTable(
            sdbus::MethodVTableItem{
                sdbus::MethodName{"Notify"},
                sdbus::Signature{"susssasa{sv}i"},
                {},
                sdbus::Signature{"u"},
                {},
                [this](sdbus::MethodCall msg) {
                  Notification notif;
                  if (handleNotifyCallDbus(msg, notif)) {
                    ctx->showNotifWindow(notif, dnd);
                  }
                },
                {},
            },
            sdbus::MethodVTableItem{sdbus::MethodName{"CloseNotification"},
                                    sdbus::Signature{"u"},
                                    {},
                                    sdbus::Signature{""},
                                    {},
                                    [this](sdbus::MethodCall msg) {
                                      handleCloseNotificationCallDbus(msg);
                                    },
                                    {}},
            sdbus::MethodVTableItem{sdbus::MethodName{"GetCapabilities"},
                                    sdbus::Signature{""},
                                    {},
                                    sdbus::Signature{"as"},
                                    {},
                                    [this](sdbus::MethodCall msg) {
                                      handleGetCapabilitiesCallDbus(msg);
                                    },
                                    {}},
            sdbus::MethodVTableItem{sdbus::MethodName{"GetServerInformation"},
                                    sdbus::Signature{""},
                                    {},
                                    sdbus::Signature{"ssss"},
                                    {},
                                    [this](sdbus::MethodCall msg) {
                                      handleGetServerInformationCallDbus(msg);
                                    },
                                    {}},
            sdbus::SignalVTableItem{sdbus::SignalName{"NotificationClosed"},
                                    sdbus::Signature{"uu"},
                                    {}})
        .forInterface(sdbus::InterfaceName{"org.freedesktop.Notifications"});
  } catch (const sdbus::Error &e) {
    std::string errMsg = "Failed to add filter for Notifications: ";
    errMsg += e.what();
    ctx->logger.LogError(TAG, errMsg);
    return;
  }

  ctx->logger.LogInfo(TAG, "Started Notification Capture Service");
}

void NotificationManager::handleGetServerInformationCallDbus(
    sdbus::MethodCall &msg) {
  try {
    sdbus::MethodReply reply = msg.createReply();

    reply << std::string{"Hyprsic"} << std::string{"Hellcat"}
          << std::string{"0.1.0"} << std::string{"1.2"};

    reply.send();
  } catch (const sdbus::Error &e) {
    std::string errMsg = "Failed to handle GetServerInformation call: ";
    errMsg += e.what();
    ctx->logger.LogError(TAG, errMsg);
  }
}

void NotificationManager::handleGetCapabilitiesCallDbus(
    sdbus::MethodCall &msg) {
  try {
    sdbus::MethodReply reply = msg.createReply();

    reply << std::vector<std::string>{"body",        "body-hyperlinks",
                                      "body-markup", "icon-static",
                                      "actions",     "persistence"};

    reply.send();
  } catch (const sdbus::Error &e) {
    std::string errMsg = "Failed to handle GetCapabilities call: ";
    errMsg += e.what();
    ctx->logger.LogError(TAG, errMsg);
  }
}

void NotificationManager::handleCloseNotificationCallDbus(
    sdbus::MethodCall &msg) {
  int32_t notifId;
  try {
    msg >> notifId;
    sdbus::MethodReply reply = msg.createReply();
    reply.send();

  } catch (const sdbus::Error &e) {
    std::string errMsg = "Failed to handle CloseNotification call: ";
    errMsg += e.what();
    ctx->logger.LogError(TAG, errMsg);
    return;
  }

  try {
    sdbus::Signal signal = dbusObject->createSignal(
        sdbus::InterfaceName{"org.freedesktop.Notifications"},
        sdbus::SignalName{"NotificationClosed"});
    signal << notifId << 2;
    dbusObject->emitSignal(signal);
  } catch (const sdbus::Error &e) {
    std::string errMsg = "Failed to emit NotificationClosed signal: ";
    errMsg += e.what();
    ctx->logger.LogError(TAG, errMsg);
  }
}

bool NotificationManager::handleNotifyCallDbus(sdbus::MethodCall &msg,
                                               Notification &notif) {
  notif.id = g_uuid_string_random();
  notif.icon = nullptr;

  try {
    // Extraction Notification Info
    msg >> notif.app_name >> notif.replaces_id >> notif.app_icon >>
        notif.summary >> notif.body >> notif.actions >> notif.hints >>
        notif.expire_timeout;

    if (notif.hints.contains("image-data")) {
      auto &imgData = notif.hints["image-data"];

      using ImgStruct = sdbus::Struct<int32_t, int32_t, int32_t, bool, int32_t,
                                      int32_t, std::vector<uint8_t>>;

      auto raw = imgData.get<ImgStruct>();

      ImageData img{raw.get<0>(), raw.get<1>(), raw.get<2>(), raw.get<3>(),
                    raw.get<4>(), raw.get<5>(), raw.get<6>()};

      Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_data(
          img.pixels.data(), Gdk::Colorspace::RGB, false, 8, img.width,
          img.height, img.rowstride);
      notif.icon = pixbuf->copy();
    } else {
      for (const auto &[key, value] : notif.hints) {
        ctx->logger.LogDebug(TAG,
                             "Hint: " + key + " = " + value.dumpToString());
      }
    }

    sdbus::MethodReply reply = msg.createReply();
    reply << notifId++;
    reply.send();
  } catch (std::exception &e) {
    ctx->logger.LogError(TAG, "Failed to handle notify call: " +
                                  std::string(e.what()));
    return false;
  }
  return true;
}
