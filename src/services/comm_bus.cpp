#include "header/comm_bus.hpp"
#include "modules/bluetooth/header/manager.hpp"
#include "modules/workspaces/hyprland/header/manager.hpp"
#include <typeindex>

CommunicationBus::CommunicationBus(AppContext *ctx, BluetoothManager *btMgr,
                                   MprisManager *mprisMgr,
                                   PulseAudioManager *paMgr,
                                   ScreenSaverManager *scrnsvrMgr,
                                   WifiManager *wifiMgr, HyprWSManager *hyprMgr)
    : ctx(ctx), btMgr(btMgr), mprisMgr(mprisMgr), paMgr(paMgr),
      scrnsvrMgr(scrnsvrMgr), wifiMgr(wifiMgr), hyprMgr(hyprMgr) {

  dispatchTable.insert(
      {std::type_index(typeid(BtRequest)), [&](const RequestMessage &msg) {
         resBus.push(btMgr->handle(std::get<BtRequest>(msg)));
       }});

  dispatchTable.insert(
      {std::type_index(typeid(MprisRequest)), [&](const RequestMessage &msg) {
         resBus.push(mprisMgr->handle(std::get<MprisRequest>(msg)));
       }});

  dispatchTable.insert(
      {std::type_index(typeid(PARequest)), [&](const RequestMessage &msg) {
         resBus.push(paMgr->handle(std::get<PARequest>(msg)));
       }});

  dispatchTable.insert(
      {std::type_index(typeid(ScrnSvrRequest)), [&](const RequestMessage &msg) {
         resBus.push(scrnsvrMgr->handle(std::get<ScrnSvrRequest>(msg)));
       }});

  dispatchTable.insert(
      {std::type_index(typeid(WifiRequest)), [&](const RequestMessage &msg) {
         resBus.push(wifiMgr->handle(std::get<WifiRequest>(msg)));
       }});

  dispatchTable.insert(
      {std::type_index(typeid(HyprRequest)), [&](const RequestMessage &msg) {
         resBus.push(hyprMgr->handle(std::get<HyprRequest>(msg)));
       }});
}
