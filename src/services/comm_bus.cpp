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

  // clang-format off
  // ── Manager Router: type_index → handler ────────────────────────────
  dispatchTable.insert(
      {std::type_index(typeid(BtRequest)), [this](const RequestWrapper &wrap) {
         auto msg = wrap.msg;
         resBus.push(ResponseWrapper{
             .msg = this->btMgr->handle(std::get<BtRequest>(msg)),
             .priority = wrap.priority});
         this->resBusCV.notify_one();
         this->ctx->logger.LogDebug(TAG, "Bluetooth Request");
       }});

  dispatchTable.insert(
      {std::type_index(typeid(MprisRequest)), [this](const RequestWrapper &wrap) {
         auto msg = wrap.msg;
         resBus.push(ResponseWrapper{
             .msg = this->mprisMgr->handle(std::get<MprisRequest>(msg)),
             .priority = wrap.priority});
         this->resBusCV.notify_one();
         this->ctx->logger.LogDebug(TAG, "Mpris Request");
       }});

  dispatchTable.insert(
      {std::type_index(typeid(PARequest)), [this](const RequestWrapper &wrap) {
         auto msg = wrap.msg;
         resBus.push(ResponseWrapper{
             .msg = this->paMgr->handle(std::get<PARequest>(msg)),
             .priority = wrap.priority});
         this->resBusCV.notify_one();
         this->ctx->logger.LogDebug(TAG, "Pulseaudio Request");
       }});

  dispatchTable.insert(
      {std::type_index(typeid(ScrnSvrRequest)),
       [this](const RequestWrapper &wrap) {
         auto msg = wrap.msg;
         resBus.push(ResponseWrapper{
             .msg = this->scrnsvrMgr->handle(std::get<ScrnSvrRequest>(msg)),
             .priority = wrap.priority});
         this->resBusCV.notify_one();
         this->ctx->logger.LogDebug(TAG, "ScreenSaver Request");
       }});

  dispatchTable.insert(
      {std::type_index(typeid(WifiRequest)), [this](const RequestWrapper &wrap) {
         auto msg = wrap.msg;
         resBus.push(ResponseWrapper{
             .msg = this->wifiMgr->handle(std::get<WifiRequest>(msg)),
             .priority = wrap.priority});
         this->resBusCV.notify_one();
         this->ctx->logger.LogDebug(TAG, "Wifi Request");
       }});

  dispatchTable.insert(
      {std::type_index(typeid(HyprRequest)), [this](const RequestWrapper &wrap) {
         auto msg = wrap.msg;
         resBus.push(ResponseWrapper{
             .msg = this->hyprMgr->handle(std::get<HyprRequest>(msg)),
             .priority = wrap.priority});
         this->resBusCV.notify_one();
         this->ctx->logger.LogDebug(TAG, "Hypr Request");
       }});

  dispatchTable.insert(
      {std::type_index(typeid(BrtRequest)), [this](const RequestWrapper &wrap) {
         auto msg = wrap.msg;
         resBus.push(ResponseWrapper{
             .msg = this->brtMgr->handle(std::get<BrtRequest>(msg)),
             .priority = wrap.priority});
         this->resBusCV.notify_one();
         this->ctx->logger.LogDebug(TAG, "Brightness Request");
       }});

  dispatchTable.insert(
      {std::type_index(typeid(SNIRequest)), [this](const RequestWrapper &wrap) {
         auto msg = wrap.msg;
         resBus.push(ResponseWrapper{
             .msg = this->sniMgr->handle(std::get<SNIRequest>(msg)),
             .priority = wrap.priority});
         this->resBusCV.notify_one();
         this->ctx->logger.LogDebug(TAG, "SNI Request");
       }});
  // clang-format on

  busThread = std::thread(&CommunicationBus::handleMessages, this);
  resThread = std::thread(&CommunicationBus::handleResponses, this);
}

CommunicationBus::~CommunicationBus() {
    
  busThread.detach();
  resThread.detach();
}

void CommunicationBus::handleMessages() {

  while (true) {
    std::unique_lock ulock(reqBusLock);

    reqBusCV.wait(ulock, [this] { return !reqBus.empty(); });

    auto evlope = reqBus.top();
    reqBus.pop();

    auto dispatcher =
        dispatchTable.find(std::visit(
            [](const auto &msg) { return std::type_index(typeid(msg)); },
            evlope.msg));

    if (dispatcher != dispatchTable.end()) {
      dispatcher->second(evlope);
    }

    ulock.unlock();
  }
}

// ── Response Router ───────────────────────────────────────────────────────
void CommunicationBus::handleResponses() {

  while (true) {
    std::unique_lock ulock(resBusLock);

    resBusCV.wait(ulock, [this] { return !resBus.empty(); });

    auto envelope = resBus.top();
    resBus.pop();

    ulock.unlock();

    // Look up which module this response belongs to
    ModuleType modType;
    {
      std::lock_guard lg(corIdLock);
      auto it = corIdToModuleType.find(envelope.msg.correlationId);
      if (it == corIdToModuleType.end()) {
        // No callback registered for this correlationId
        this->ctx->logger.LogWarning(
            TAG, "No callback registered for correlationId: " +
                     std::to_string(envelope.msg.correlationId));
        continue;
      }
      modType = it->second;
      corIdToModuleType.erase(it);
    }

    // Bus Router → Module Router
    std::lock_guard cbGuard(cbLock);
    auto it = moduleCBs.find(modType);
    if (it != moduleCBs.end()) {
      for (const auto &cb : it->second) {
        // NOTE: Runs on Bus thread — use g_idle_add() for GTK updates
        cb(envelope.msg);
      }
    }
  }
}

void CommunicationBus::SendMessage(RequestMessage msg, Priority priority) {
  std::lock_guard lg(reqBusLock);
  reqBus.push({.msg = msg, .priority = priority});

  ctx->logger.LogDebug(TAG, "Request Received... Total Message:" +
                               std::to_string(reqBus.size()));
  reqBusCV.notify_one();
}

void CommunicationBus::SendMessage(RequestMessage msg, Priority priority,
                                    ModuleType modType) {
  uint64_t corId = GetNewCorId();

  // Store the mapping so handleResponses knows which module to route to
  {
    std::lock_guard lg(corIdLock);
    corIdToModuleType[corId] = modType;
  }

  // Inject correlationId into the request. Since it's a variant, we use visit.
  // But std::visit on a temporary doesn't work well. We apply to the local msg.
  std::visit(
      [corId](auto &req) { 
        using T = std::decay_t<decltype(req)>;
        if constexpr (requires(T t) { t.correlationId; }) {
          req.correlationId = corId;
        }
      },
      msg);

  // Push to request queue
  {
    std::lock_guard lg(reqBusLock);
    reqBus.push({.msg = std::move(msg), .priority = priority});
  }

  ctx->logger.LogDebug(TAG, "Request Received (with ModuleType)... Total Message: " +
                               std::to_string(reqBus.size()));
  reqBusCV.notify_one();
}

void CommunicationBus::RegisterCallback(
    ModuleType mod, std::function<void(ResponseMessage)> cb) {
  std::lock_guard lg(cbLock);
  moduleCBs[mod].push_back(std::move(cb));
}

uint64_t CommunicationBus::GetNewCorId() { return idCounter++; }
