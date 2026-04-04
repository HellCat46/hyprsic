#include "header/comm_bus.hpp"
#include "modules/bluetooth/header/manager.hpp"
#include "modules/brightness/header/manager.hpp"
#include "modules/statusnotifier/header/manager.hpp"
#include "modules/workspaces/hyprland/header/manager.hpp"
#include "services/header/comm_types.hpp"
#include "services/header/context.hpp"
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <typeindex>
#include <variant>
#define TAG "Communication Bus"

CommunicationBus::CommunicationBus(AppContext *ctx, BluetoothManager *btMgr,
                                   MprisManager *mprisMgr,
                                   PulseAudioManager *paMgr,
                                   ScreenSaverManager *scrnsvrMgr,
                                   WifiManager *wifiMgr, HyprWSManager *hyprMgr,
                                   BrightnessManager *brtMgr,
                                   StatusNotifierManager *sniMgr)
    : ctx(ctx), btMgr(btMgr), mprisMgr(mprisMgr), paMgr(paMgr),
      scrnsvrMgr(scrnsvrMgr), wifiMgr(wifiMgr), hyprMgr(hyprMgr),
      sniMgr(sniMgr), idCounter(0) {

  dispatchTable.insert(
      {std::type_index(typeid(BtRequest)), [this](const RequestWrapper &wrap) {
         resBus.push(ResponseWrapper{
             .msg = this->btMgr->handle(std::get<BtRequest>(wrap.msg)),
             .priority = wrap.priority});

         this->ctx->logger.LogDebug(TAG, "Bluetooth Request");
       }});

  dispatchTable.insert(
      {std::type_index(typeid(MprisRequest)), [this](const RequestWrapper &wrap) {
         resBus.push(ResponseWrapper{
             .msg = this->mprisMgr->handle(std::get<MprisRequest>(wrap.msg)),
             .priority = wrap.priority});

         this->ctx->logger.LogDebug(TAG, "Mpris Request");
       }});

  dispatchTable.insert(
      {std::type_index(typeid(PARequest)), [this](const RequestWrapper &wrap) {
         resBus.push(ResponseWrapper{
             .msg = this->paMgr->handle(std::get<PARequest>(wrap.msg)),
             .priority = wrap.priority});

         this->ctx->logger.LogDebug(TAG, "Pulseaudio Request");
       }});

  dispatchTable.insert(
      {std::type_index(typeid(ScrnSvrRequest)),
       [this](const RequestWrapper &wrap) {
         resBus.push(ResponseWrapper{
             .msg = this->scrnsvrMgr->handle(std::get<ScrnSvrRequest>(wrap.msg)),
             .priority = wrap.priority});

         this->ctx->logger.LogDebug(TAG, "ScreenSaver Request");
       }});

  dispatchTable.insert(
      {std::type_index(typeid(WifiRequest)), [this](const RequestWrapper &wrap) {
         resBus.push(ResponseWrapper{
             .msg = this->wifiMgr->handle(std::get<WifiRequest>(wrap.msg)),
             .priority = wrap.priority});

         this->ctx->logger.LogDebug(TAG, "Wifi Request");
       }});

  dispatchTable.insert(
      {std::type_index(typeid(HyprRequest)), [this](const RequestWrapper &wrap) {
         resBus.push(ResponseWrapper{
             .msg = this->hyprMgr->handle(std::get<HyprRequest>(wrap.msg)),
             .priority = wrap.priority});

         this->ctx->logger.LogDebug(TAG, "Hypr Request");
       }});

  dispatchTable.insert(
      {std::type_index(typeid(BrtRequest)), [this](const RequestWrapper &wrap) {
         resBus.push(ResponseWrapper{
             .msg = this->brtMgr->handle(std::get<BrtRequest>(wrap.msg)),
             .priority = wrap.priority});

         this->ctx->logger.LogDebug(TAG, "Brightness Request");
       }});

  dispatchTable.insert(
      {std::type_index(typeid(SNIRequest)), [this](const RequestWrapper &wrap) {
         resBus.push(ResponseWrapper{
             .msg = this->sniMgr->handle(std::get<SNIRequest>(wrap.msg)),
             .priority = wrap.priority});

         this->ctx->logger.LogDebug(TAG, "SNI Request");
       }});

  busThread = std::thread(&CommunicationBus::handleMessages, this);
}

void CommunicationBus::handleMessages() {

  while (true) {
    std::unique_lock ulock(reqBusLock);

    reqBusCV.wait(ulock, [this] { return !reqBus.empty(); });

    auto evlope = reqBus.top();
    reqBus.pop();

    auto dispatcher = dispatchTable.find(
        std::visit([](const auto &msg) { return std::type_index(typeid(msg)); },
                   evlope.msg));

    if (dispatcher != dispatchTable.end()) {
      dispatcher->second(evlope);
    }

    ulock.unlock();
  }
}

void CommunicationBus::SendMessage(RequestMessage msg, Priority priority) {
  std::lock_guard lg(reqBusLock);
  reqBus.push({.msg = msg, .priority = priority});

  ctx->logger.LogDebug(TAG, "Request Received... Total Message:" +
                               std::to_string(reqBus.size()));
  reqBusCV.notify_one();
}

uint64_t CommunicationBus::GetNewCorId() { return idCounter++; }
