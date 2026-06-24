#include "header/manager.hpp"
#include "services/header/comm_types.hpp"
#include <sdbus-c++/Error.h>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/Types.h>
#include <string>

#define TAG "ScreenSaverManager"

ScreenSaverManager::ScreenSaverManager(AppContext *ctx) : ctx(ctx) {
  try {
    dbusProxy = sdbus::createProxy(
        *ctx->dbus.ssnConn, sdbus::ServiceName{"org.freedesktop.ScreenSaver"},
        sdbus::ObjectPath{"/org/freedesktop/ScreenSaver"});
  } catch (const sdbus::Error &e) {
    this->ctx->logger.LogError(TAG, "Failed to create D-Bus proxy: " +
                                        std::string(e.what()));
  }
}

ResponseMessage
ScreenSaverManager::activateScreenSaver(const ScrnSvrActivateRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  try {
    dbusProxy->callMethod(sdbus::MethodName{"Inhibit"})
        .onInterface("org.freedesktop.ScreenSaver")
        .withArguments(std::string{"Hyprsic"},
                       std::string{"Activating Screen Saver"})
        .storeResultsTo(inhibitCookie);
  } catch (const sdbus::Error &e) {
    resp.errMsg = std::string{"Failed to activate screen saver: "} + e.what();
    this->ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  resp.success = true;
  return resp;
}

ResponseMessage
ScreenSaverManager::deactivateScreenSaver(const ScrnSvrDeActivateRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  try {
    dbusProxy->callMethod(sdbus::MethodName{"UnInhibit"})
        .onInterface("org.freedesktop.ScreenSaver")
        .withArguments(inhibitCookie);

    inhibitCookie = -1;
  } catch (const sdbus::Error &e) {
    resp.errMsg = std::string{"Failed to deactivate screen saver: "} + e.what();
    this->ctx->logger.LogError(TAG, resp.errMsg);
    return resp;
  }

  resp.success = true;
  return resp;
}

bool ScreenSaverManager::isActive() { return inhibitCookie != -1; }

ResponseMessage ScreenSaverManager::handle(const ScrnSvrRequest &req) {
  ResponseMessage resp;

  std::visit(
      [&](auto &reqMsg) {
        using T = std::decay_t<decltype(reqMsg)>;

        if constexpr (std::is_same_v<T, ScrnSvrActivateRequest>) {
          resp = activateScreenSaver(reqMsg);
        } else if constexpr (std::is_same_v<T, ScrnSvrDeActivateRequest>) {
          resp = deactivateScreenSaver(reqMsg);
        }
      },
      req);

  return resp;
}
