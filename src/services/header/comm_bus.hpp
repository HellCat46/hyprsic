#pragma once

#include "modules/bluetooth/header/manager.hpp"
#include "modules/mpris/header/manager.hpp"
#include "modules/pulseaudio/header/manager.hpp"
#include "modules/screensaver/header/manager.hpp"
#include "modules/wifi/header/manager.hpp"
#include "modules/workspaces/hyprland/header/manager.hpp"
#include "services/header/context.hpp"
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <typeindex>
#include <unordered_map>
#include <variant>

using RequestMessage = std::variant<BtRequest, MprisRequest, PARequest,
                                    ScrnSvrRequest, WifiRequest, HyprRequest>;

class CommunicationBus {
  AppContext *ctx;
  BluetoothManager *btMgr;
  MprisManager *mprisMgr;
  PulseAudioManager *paMgr;
  ScreenSaverManager *scrnsvrMgr;
  WifiManager *wifiMgr;
  HyprWSManager *hyprMgr;

  // Request Queue
  std::priority_queue<RequestMessage> reqBus;
  std::mutex reqBusLock;
  std::condition_variable reqBusCV;

  // Response Queue
  std::priority_queue<ResponseMessage> resBus;
  std::mutex resBusLock;
  std::condition_variable resBusCV;

  std::thread busThread;

  std::unordered_map<std::type_index,
                     std::function<void(const RequestMessage &msg)>>
      dispatchTable;

public:
  CommunicationBus(AppContext *ctx, BluetoothManager *btMgr,
                   MprisManager *mprisMgr, PulseAudioManager *paMgr,
                   ScreenSaverManager *scrnsvrMgr, WifiManager *wifiMgr,
                   HyprWSManager *hyprMgr);
  
  
};
