#include "manager.hpp"
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/operation.h>
#include <pulse/subscribe.h>
#include <string>

#define TAG "PulseAudioManager"

PulseAudioManager::PulseAudioManager(LoggingManager *logMgr) : logger(logMgr) {

  mainLoop = pa_threaded_mainloop_new();
  if (mainLoop == nullptr) {
    logger->LogError(TAG, "Failed to Get Pulseaudio Main Loop.");
    return;
  }

  pa_mainloop_api *mainLoopAPI = pa_threaded_mainloop_get_api(mainLoop);
  pulseContext = pa_context_new(mainLoopAPI, "hyprsic");
  if (!pulseContext) {
    logger->LogError(TAG, "Failed to Create a Pulseaudio Context.");
    return;
  }

  pa_context_set_state_callback(pulseContext, contextStateHandler, this);

  if (pa_context_connect(pulseContext, nullptr, PA_CONTEXT_NOFAIL, nullptr) <
      0) {
    logger->LogError(TAG, "Failed to Connect the Pulseaudio Context.");
    return;
  }

  if (pa_threaded_mainloop_start(mainLoop) < 0) {
    logger->LogError(TAG, "Failed to Start the Pulseaudio Context.");
    return;
  }

  return;
}

void PulseAudioManager::contextStateHandler(pa_context *pulseCtx, void *data) {
  PulseAudioManager *self = static_cast<PulseAudioManager *>(data);

  switch (pa_context_get_state(pulseCtx)) {
  case PA_CONTEXT_READY:
    self->logger->LogInfo(
        TAG, "PulseAudio Connection Established. Subscribing to Events...");
    pa_context_get_server_info(pulseCtx, serverInfoCallBack, data);

    pa_context_set_subscribe_callback(pulseCtx, handleStateChanges, data);
    pa_context_subscribe(
        pulseCtx,
        (enum pa_subscription_mask)(PA_SUBSCRIPTION_EVENT_SERVER |
                                    PA_SUBSCRIPTION_EVENT_SOURCE |
                                    PA_SUBSCRIPTION_EVENT_SINK),
        nullptr, nullptr);
    self->logger->LogInfo(TAG, "Successfully Subscribed to Pulseaudio Events.");
    self->getDevices();
    break;
  case PA_CONTEXT_TERMINATED:
    self->logger->LogInfo(TAG, "Connection Terminated");
    break;
  case PA_CONTEXT_FAILED:
    self->logger->LogError(TAG, "Connection Failed");
    break;
  default:
    break;
  }
}

void PulseAudioManager::serverInfoCallBack(pa_context *pulseCtx,
                                           const pa_server_info *info,
                                           void *data) {
  PulseAudioManager *playing = (PulseAudioManager *)data;

  playing->defInput = info->default_source_name;
  playing->defOutput = info->default_sink_name;
}

void PulseAudioManager::handleStateChanges(
    pa_context *pulseCtx, const pa_subscription_event_type eventType,
    unsigned int idx, void *data) {
  unsigned int facility = eventType & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;

  switch (facility) {

  case PA_SUBSCRIPTION_EVENT_SERVER:
    pa_context_get_server_info(pulseCtx, serverInfoCallBack, data);
    break;

  case PA_SUBSCRIPTION_EVENT_SOURCE:
    pa_context_get_source_info_list(pulseCtx, sourceInfoCallBack, data);
    break;

  case PA_SUBSCRIPTION_EVENT_SINK:
    pa_context_get_sink_info_list(pulseCtx, sinkInfoCallBack, data);
    break;
  }
}

void PulseAudioManager::sinkInfoCallBack(pa_context *pulseCtx,
                                         const pa_sink_info *info, int eol,
                                         void *data) {
  if (info == nullptr)
    return;

  PulseAudioManager *self = static_cast<PulseAudioManager *>(data);

  auto it = self->outDevs.find(info->name);
  if (it != self->outDevs.end()) {
    // Device already exists, update its information
    it->second.index = info->index;
    it->second.description = info->description;
    it->second.mute = info->mute ? true : false;
    it->second.channels = info->volume.channels;
    it->second.volume.clear();
    for (int i = 0; i < info->volume.channels; i++) {
      it->second.volume.push_back(info->volume.values[i]);
    }
    return;
  }

  PulseAudioDevice dev{info->index, info->description,
                       info->mute ? true : false, info->volume.channels,
                       std::vector<uint32_t>()};

  for (int i = 0; i < info->volume.channels; i++) {
    dev.volume.push_back(info->volume.values[i]);
  }
  self->outDevs.insert({info->name, dev});
  self->logger->LogDebug(
      TAG, "Sink Device Found: " + std::string(info->name) +
               " | Description: " + dev.description + "(Total " +
               std::to_string(self->outDevs.size()) + " Devices)");
}

void PulseAudioManager::sourceInfoCallBack(pa_context *pulseCtx,
                                           const pa_source_info *info, int eol,
                                           void *data) {
  if (info == nullptr)
    return;

  PulseAudioManager *self = static_cast<PulseAudioManager *>(data);

  auto it = self->inDevs.find(info->name);
  if (it != self->inDevs.end()) {
    // Device already exists, update its information
    it->second.index = info->index;
    it->second.description = info->description;
    it->second.mute = info->mute ? true : false;
    it->second.channels = info->volume.channels;
    it->second.volume.clear();
    for (int i = 0; i < info->volume.channels; i++) {
      it->second.volume.push_back(info->volume.values[i]);
    }
    return;
  }

  PulseAudioDevice dev{info->index, info->description,
                       info->mute ? true : false, info->volume.channels,
                       std::vector<uint32_t>()};

  for (int i = 0; i < info->volume.channels; i++) {
    dev.volume.push_back(info->volume.values[i]);
  }
  self->inDevs.insert({info->name, dev});
  self->logger->LogInfo(TAG,
                        "Source Device Found: " + std::string(info->name) +
                            " | Description: " + dev.description + "(Total " +
                            std::to_string(self->inDevs.size()) + " Devices)");
}

void PulseAudioManager::getDevices() {
  pa_context_get_sink_info_list(pulseContext, sinkInfoCallBack, this);
  pa_context_get_source_info_list(pulseContext, sourceInfoCallBack, this);
}

void PulseAudioManager::setVolume(const std::string &devName, bool isOutput,
                                  uint32_t volume) {
  pa_cvolume paVolume;
  pa_cvolume_set(&paVolume, 2, (uint32_t)((float)volume / 100 * 65535));

  if (isOutput) {
    auto it = outDevs.find(devName);
    if (it == outDevs.end())
      return;

    pa_threaded_mainloop_lock(mainLoop);
    auto op = pa_context_set_sink_volume_by_index(
        pulseContext, it->second.index, &paVolume, nullptr, nullptr);
    pa_threaded_mainloop_unlock(mainLoop);

    if (!op)
      return;

    pa_operation_unref(op);
  } else {
    auto it = inDevs.find(devName);
    if (it == inDevs.end())
      return;

    pa_threaded_mainloop_lock(mainLoop);
    auto op = pa_context_set_source_volume_by_index(
        pulseContext, it->second.index, &paVolume, nullptr, nullptr);
    pa_threaded_mainloop_unlock(mainLoop);

    if (!op)
      return;

    pa_operation_unref(op);
  }
}

short PulseAudioManager::toggleMute(const std::string &devName, bool isOutput) {

  if (isOutput) {
    auto it = outDevs.find(devName);
    if (it == outDevs.end())
      return -1;

    pa_threaded_mainloop_lock(mainLoop);
    auto op = pa_context_set_sink_mute_by_index(
        pulseContext, it->second.index, !it->second.mute, nullptr, nullptr);
    pa_threaded_mainloop_lock(mainLoop);

    if (!op)
      return -1;

    it->second.mute = !it->second.mute;
    pa_operation_unref(op);
    return it->second.mute;
  } else {
    auto it = inDevs.find(devName);
    if (it == inDevs.end())
      return -1;

    pa_threaded_mainloop_lock(mainLoop);
    auto op = pa_context_set_source_mute_by_index(
        pulseContext, it->second.index, !it->second.mute, nullptr, nullptr);
    pa_threaded_mainloop_lock(mainLoop);

    if (!op)
      return -1;

    it->second.mute = !it->second.mute;
    pa_operation_unref(op);
    return it->second.mute;
  }
}

bool PulseAudioManager::updateDefDevice(const std::string &devName,
                                        bool isOutput) {
  if (isOutput) {
    auto it = outDevs.find(devName);
    if (it != outDevs.end()) {

      pa_threaded_mainloop_lock(mainLoop);
      auto op = pa_context_set_default_sink(pulseContext, devName.c_str(),
                                            nullptr, nullptr);
      pa_threaded_mainloop_lock(mainLoop);

      if (!op)
        return false;

      pa_operation_unref(op);
      defOutput = devName;
      return true;
    }
  } else {

    auto it = inDevs.find(devName);
    if (it != inDevs.end()) {

      pa_threaded_mainloop_lock(mainLoop);
      auto op = pa_context_set_default_source(pulseContext, devName.c_str(),
                                              nullptr, nullptr);
      pa_threaded_mainloop_lock(mainLoop);

      if (!op)
        return false;

      pa_operation_unref(op);
      defInput = devName;
      return true;
    }
  }
  return false;
}
