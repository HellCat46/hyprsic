#pragma once
#include "../../logging/manager.hpp"
#include "string"
#include <cstdint>
#include <map>
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/thread-mainloop.h>
#include <string>
#include <vector>

struct PulseAudioDevice {
  uint32_t index;
  std::string description;
  bool mute;
  uint8_t channels;
  std::vector<uint32_t> volume;
};

class PulseAudioManager {
  pa_context *pulseContext;
  pa_threaded_mainloop *mainLoop;
  LoggingManager *logger;

  static void contextStateHandler(pa_context *, void *);
  static void handleStateChanges(pa_context *, const pa_subscription_event_type,
                                 unsigned int, void *);

  static void serverInfoCallBack(pa_context *, const pa_server_info *, void *);
  static void sinkInfoCallBack(pa_context *pulseCtx, const pa_sink_info *info,
                               int eol, void *data);
  static void sourceInfoCallBack(pa_context *pulseCtx,
                                 const pa_source_info *info, int eol,
                                 void *data);

public:
  std::string defOutput, defInput;
  std::map<std::string, PulseAudioDevice> outDevs, inDevs;
  PulseAudioManager(LoggingManager *logMgr);
  ~PulseAudioManager();
  void getDevices();
  
  void setVolume(const std::string &devName, bool isOutput, uint32_t volume);
  short toggleMute(const std::string &devName, bool isOutput);
  bool updateDefDevice(const std::string &devName, bool isOutput);
};
