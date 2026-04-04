#pragma once

#include "modules/bluetooth/header/manager.hpp"
#include "modules/brightness/header/manager.hpp"
#include "modules/mpris/header/manager.hpp"
#include "modules/pulseaudio/header/manager.hpp"
#include "modules/screensaver/header/manager.hpp"
#include "modules/statusnotifier/header/manager.hpp"
#include "modules/wifi/header/manager.hpp"
#include "modules/workspaces/hyprland/header/manager.hpp"
#include "services/header/comm_types.hpp"
#include "services/header/context.hpp"
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <typeindex>
#include <unordered_map>
#include <variant>
#include <vector>

using RequestMessage = std::variant<BtRequest, MprisRequest, PARequest,
                                    ScrnSvrRequest, WifiRequest, HyprRequest, BrtRequest, SNIRequest>;

struct RequestWrapper {
  RequestMessage msg;
  Priority priority;
};

struct RequestWrapperCmp {
  bool operator()(const RequestWrapper &a, const RequestWrapper &b) const {
    return a.priority < b.priority;
  }
};

class CommunicationBus {
    AppContext*  ctx;
  BluetoothManager *btMgr;
  MprisManager *mprisMgr;
  PulseAudioManager *paMgr;
  ScreenSaverManager *scrnsvrMgr;
  WifiManager *wifiMgr;
  HyprWSManager *hyprMgr;
  BrightnessManager *brtMgr;
  StatusNotifierManager *sniMgr;
  
  uint64_t idCounter;

  // Request Queue
  std::priority_queue<RequestWrapper, std::vector<RequestWrapper>,
                      RequestWrapperCmp>
      reqBus;
  std::mutex reqBusLock;
  std::condition_variable reqBusCV;

  // Response Queue
  std::priority_queue<ResponseWrapper, std::vector<ResponseWrapper>,
                      ResponseWrapperCmp>
      resBus;
  std::mutex resBusLock;
  std::condition_variable resBusCV;

  std::thread busThread;
  void handleMessages();

  std::unordered_map<std::type_index,
                     std::function<void(const RequestWrapper &msg)>>
      dispatchTable;

public:
  CommunicationBus(AppContext *ctx,BluetoothManager *btMgr, MprisManager *mprisMgr,
             PulseAudioManager *paMgr, ScreenSaverManager *scrnsvrMgr,
             WifiManager *wifiMgr, HyprWSManager *hyprMgr, BrightnessManager* brtMgr, StatusNotifierManager* sniMgr);

  void SendMessage(RequestMessage msg, Priority priority);
  
  uint64_t GetNewCorId();
};
