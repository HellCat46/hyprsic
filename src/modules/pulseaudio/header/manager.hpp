#pragma once
#include "services/header/comm_types.hpp"
#include "services/header/context.hpp"
#include "string"
#include <cstdint>
#include <map>
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/thread-mainloop.h>
#include <string>
#include <variant>
#include <vector>

struct PulseAudioDevice {
  uint32_t index;
  std::string description;
  bool mute;
  uint8_t channels;
  std::vector<uint32_t> volume;
};

struct PASetVolumeRequest {
    std::string devName;
    bool isOutput;
    uint32_t volume;
    
    uint64_t correlationId;
};

struct PAToggleMuteRequest {
    std::string devName;
    bool isOutput;
    
    uint64_t correlationId;
};

struct PAUpdateDefDeviceRequest {
    std::string devName;
    bool isOutput;
    
    uint64_t correlationId;
};

using PARequest = std::variant<PASetVolumeRequest, PAToggleMuteRequest, PAUpdateDefDeviceRequest>;
class PulseAudioManager {
  AppContext *ctx;
  pa_context *pulseCtx;
  pa_threaded_mainloop *mainLoop;

  static void contextStateHandler(pa_context *, void *);
  static void handleStateChanges(pa_context *, const pa_subscription_event_type,
                                 unsigned int, void *);

  static void serverInfoCallBack(pa_context *, const pa_server_info *, void *);
  static void sinkInfoCallBack(pa_context *pulseCtx, const pa_sink_info *info,
                               int eol, void *data);
  static void sourceInfoCallBack(pa_context *pulseCtx,
                                 const pa_source_info *info, int eol,
                                 void *data);

  ResponseMessage setVolume(PASetVolumeRequest req);
  ResponseMessage toggleMute(PAToggleMuteRequest req);
  ResponseMessage updateDefDevice(PAUpdateDefDeviceRequest req);

  void getDevices();

public:
  PulseAudioManager(AppContext *ctx);

  std::string defOutput, defInput;
  std::map<std::string, PulseAudioDevice> outDevs, inDevs;
  
  ResponseMessage handle(const PARequest& msg);

  ~PulseAudioManager();
};
