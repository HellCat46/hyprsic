#include "header/comm_bus.hpp"
#include "modules/bluetooth/header/manager.hpp"
#include "modules/brightness/header/manager.hpp"
#include "modules/workspaces/hyprland/header/manager.hpp"
#include "services/header/comm_types.hpp"
#include <cstdint>
#include <mutex>
#include <thread>
#include <typeindex>

CommunicationBus::CommunicationBus(BluetoothManager *btMgr,
                                   MprisManager *mprisMgr,
                                   PulseAudioManager *paMgr,
                                   ScreenSaverManager *scrnsvrMgr,
                                   WifiManager *wifiMgr, HyprWSManager *hyprMgr,
                                   BrightnessManager *brtMgr)
    : btMgr(btMgr), mprisMgr(mprisMgr), paMgr(paMgr), scrnsvrMgr(scrnsvrMgr),
      wifiMgr(wifiMgr), hyprMgr(hyprMgr), idCounter(0) {

  dispatchTable.insert(
      {std::type_index(typeid(BtRequest)), [&](const RequestWrapper &wrap) {
         resBus.push(ResponseWrapper{
             .msg = btMgr->handle(std::get<BtRequest>(wrap.msg)),
             .priority = wrap.priority});
       }});

  dispatchTable.insert(
      {std::type_index(typeid(MprisRequest)), [&](const RequestWrapper &wrap) {
         resBus.push(ResponseWrapper{
             .msg = mprisMgr->handle(std::get<MprisRequest>(wrap.msg)),
             .priority = wrap.priority});
       }});

  dispatchTable.insert(
      {std::type_index(typeid(PARequest)), [&](const RequestWrapper &wrap) {
         resBus.push(ResponseWrapper{
             .msg = paMgr->handle(std::get<PARequest>(wrap.msg)),
             .priority = wrap.priority});
       }});

  dispatchTable.insert(
      {std::type_index(typeid(ScrnSvrRequest)),
       [&](const RequestWrapper &wrap) {
         resBus.push(ResponseWrapper{
             .msg = scrnsvrMgr->handle(std::get<ScrnSvrRequest>(wrap.msg)),
             .priority = wrap.priority});
       }});

  dispatchTable.insert(
      {std::type_index(typeid(WifiRequest)), [&](const RequestWrapper &wrap) {
         resBus.push(ResponseWrapper{
             .msg = wifiMgr->handle(std::get<WifiRequest>(wrap.msg)),
             .priority = wrap.priority});
       }});

  dispatchTable.insert(
      {std::type_index(typeid(HyprRequest)), [&](const RequestWrapper &wrap) {
         resBus.push(ResponseWrapper{
             .msg = hyprMgr->handle(std::get<HyprRequest>(wrap.msg)),
             .priority = wrap.priority});
       }});

  dispatchTable.insert(
      {std::type_index(typeid(BrtRequest)), [&](const RequestWrapper &wrap) {
         resBus.push(ResponseWrapper{.msg = brtMgr->handle(std::get<BrtRequest>(wrap.msg)), .priority = wrap.priority})
       }});

  busThread = std::thread(&CommunicationBus::handleMessages, this);
}

void CommunicationBus::handleMessages() {

  std::unique_lock ulock(reqBusLock);
  while (true) {
    reqBusCV.wait(ulock, [] {});
  }
}

void CommunicationBus::SendMessage(RequestMessage msg, Priority priority) {
  std::lock_guard lg(reqBusLock);
  reqBus.push(RequestWrapper{.msg = msg, .priority = priority});

  reqBusCV.notify_one();
}

uint64_t CommunicationBus::GetNewCorId() { return idCounter++; }
