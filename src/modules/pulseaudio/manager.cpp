#include "header/manager.hpp"
#include "services/header/comm_types.hpp"
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/operation.h>
#include <pulse/subscribe.h>
#include <pulse/thread-mainloop.h>
#include <string>

#define TAG "PulseAudioManager"

PulseAudioManager::PulseAudioManager(AppContext *ctx) : ctx(ctx) {

  mainLoop = pa_threaded_mainloop_new();
  if (mainLoop == nullptr) {
    ctx->logger.LogError(TAG, "Failed to Get Pulseaudio Main Loop.");
    return;
  }

  pa_mainloop_api *mainLoopAPI = pa_threaded_mainloop_get_api(mainLoop);
  pulseCtx = pa_context_new(mainLoopAPI, "hyprsic");
  if (!pulseCtx) {
    ctx->logger.LogError(TAG, "Failed to Create a Pulseaudio Context.");
    return;
  }

  pa_context_set_state_callback(pulseCtx, contextStateHandler, this);

  if (pa_context_connect(pulseCtx, nullptr, PA_CONTEXT_NOFAIL, nullptr) < 0) {
    ctx->logger.LogError(TAG, "Failed to Connect the Pulseaudio Context.");
    return;
  }

  if (pa_threaded_mainloop_start(mainLoop) < 0) {
    ctx->logger.LogError(TAG, "Failed to Start the Pulseaudio Context.");
    return;
  }

  return;
}

void PulseAudioManager::contextStateHandler(pa_context *pulseCtx, void *data) {
  PulseAudioManager *self = static_cast<PulseAudioManager *>(data);

  switch (pa_context_get_state(pulseCtx)) {
  case PA_CONTEXT_READY:
    self->ctx->logger.LogInfo(
        TAG, "PulseAudio Connection Established. Subscribing to Events...");
    pa_context_get_server_info(pulseCtx, serverInfoCallBack, data);

    pa_context_set_subscribe_callback(pulseCtx, handleStateChanges, data);
    pa_context_subscribe(
        pulseCtx,
        (enum pa_subscription_mask)(PA_SUBSCRIPTION_EVENT_SERVER |
                                    PA_SUBSCRIPTION_EVENT_SOURCE |
                                    PA_SUBSCRIPTION_EVENT_SINK),
        nullptr, nullptr);
    self->ctx->logger.LogInfo(TAG,
                              "Successfully Subscribed to Pulseaudio Events.");
    break;
  case PA_CONTEXT_TERMINATED:
    self->ctx->logger.LogInfo(TAG, "Connection Terminated");
    break;
  case PA_CONTEXT_FAILED:
    self->ctx->logger.LogError(TAG, "Connection Failed");
    break;
  default:
    break;
  }
}

void PulseAudioManager::serverInfoCallBack(
    [[maybe_unused]] pa_context *pulseCtx, const pa_server_info *info,
    void *data) {
  PulseAudioManager *playing = (PulseAudioManager *)data;

  playing->defInput = info->default_source_name;
  playing->defOutput = info->default_sink_name;
}

void PulseAudioManager::handleStateChanges(
    pa_context *pulseCtx, const pa_subscription_event_type eventType,
    [[maybe_unused]] unsigned int idx, void *data) {
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

void PulseAudioManager::sinkInfoCallBack([[maybe_unused]] pa_context *pulseCtx,
                                         const pa_sink_info *info,
                                         [[maybe_unused]] int eol, void *data) {
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
  self->ctx->logger.LogDebug(
      TAG, "Sink Device Found: " + std::string(info->name) +
               " | Description: " + dev.description + "(Total " +
               std::to_string(self->outDevs.size()) + " Devices)");
}

void PulseAudioManager::sourceInfoCallBack(
    [[maybe_unused]] pa_context *pulseCtx, const pa_source_info *info,
    [[maybe_unused]] int eol, void *data) {
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
  self->ctx->logger.LogInfo(
      TAG, "Source Device Found: " + std::string(info->name) +
               " | Description: " + dev.description + "(Total " +
               std::to_string(self->inDevs.size()) + " Devices)");
}

void PulseAudioManager::updateDevices() {
  pa_threaded_mainloop_lock(mainLoop);
  pa_context_get_sink_info_list(pulseCtx, sinkInfoCallBack, this);
  pa_context_get_source_info_list(pulseCtx, sourceInfoCallBack, this);
  pa_threaded_mainloop_unlock(mainLoop);
}

ResponseMessage PulseAudioManager::setVolume(const PASetVolumeRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  pa_cvolume paVolume;
  pa_cvolume_set(&paVolume, 2, (uint32_t)((float)req.volume / 100 * 65535));

  pa_threaded_mainloop_lock(mainLoop);

  if (req.isOutput) {
    auto it = outDevs.find(req.devName);
    if (it == outDevs.end()) {
      pa_threaded_mainloop_unlock(mainLoop);

      resp.errMsg =
          "Failed to Set Volume. Output Device Not Found: " + req.devName;
      return resp;
    }

    auto op = pa_context_set_sink_volume_by_index(pulseCtx, it->second.index,
                                                  &paVolume, nullptr, nullptr);

    if (op)
      pa_operation_unref(op);

  } else {
    auto it = inDevs.find(req.devName);
    if (it == inDevs.end()) {
      pa_threaded_mainloop_unlock(mainLoop);

      resp.errMsg =
          "Failed to Set Volume. Input Device Not Found: " + req.devName;
      return resp;
    }

    auto op = pa_context_set_source_volume_by_index(
        pulseCtx, it->second.index, &paVolume, nullptr, nullptr);

    if (op)
      pa_operation_unref(op);
  }

  pa_threaded_mainloop_unlock(mainLoop);

  resp.success = true;
  return resp;
}

ResponseMessage PulseAudioManager::toggleMute(const PAToggleMuteRequest &req) {
  ResponseMessage resp{
      .success = false, .errMsg = "", .correlationId = req.correlationId};

  pa_threaded_mainloop_lock(mainLoop);

  if (req.isOutput) {
    auto it = outDevs.find(req.devName);
    if (it == outDevs.end()) {
      pa_threaded_mainloop_unlock(mainLoop);

      resp.errMsg =
          "Failed to Toggle Mute. Output Device Not Found: " + req.devName;
      return resp;
    }

    auto op = pa_context_set_sink_mute_by_index(
        pulseCtx, it->second.index, !it->second.mute, nullptr, nullptr);

    if (op) {
      it->second.mute = !it->second.mute;
      pa_operation_unref(op);
      resp.success = it->second.mute;
    }
  } else {
    auto it = inDevs.find(req.devName);
    if (it == inDevs.end()) {
      pa_threaded_mainloop_unlock(mainLoop);

      resp.errMsg =
          "Failed to Toggle Mute. Input Device Not Found: " + req.devName;
      return resp;
    }

    auto op = pa_context_set_source_mute_by_index(
        pulseCtx, it->second.index, !it->second.mute, nullptr, nullptr);

    if (op) {
      it->second.mute = !it->second.mute;
      pa_operation_unref(op);
      resp.success = it->second.mute;
    }
  }

  pa_threaded_mainloop_unlock(mainLoop);

  return resp;
}

ResponseMessage
PulseAudioManager::updateDefDevice(const PAUpdateDefDeviceRequest &req) {
  ResponseMessage resp{.success = false,
                       .errMsg = "Failed to update default device",
                       .correlationId = req.correlationId};
  

  pa_threaded_mainloop_lock(mainLoop);

  if (req.isOutput) {
    auto it = outDevs.find(req.devName);
    if (it != outDevs.end()) {

      auto op = pa_context_set_default_sink(pulseCtx, req.devName.c_str(),
                                            nullptr, nullptr);
      if (op) {
        pa_operation_unref(op);
        defOutput = req.devName;
        resp.success = true;
      }
    }
  } else {

    auto it = inDevs.find(req.devName);
    if (it != inDevs.end()) {

      auto op = pa_context_set_default_source(pulseCtx, req.devName.c_str(),
                                              nullptr, nullptr);
      if (op) {
        pa_operation_unref(op);
        defInput = req.devName;
        resp.success = true;
      }
    }
  }

  pa_threaded_mainloop_unlock(mainLoop);
  return resp;
}

PulseAudioManager::~PulseAudioManager() {
  if (mainLoop) {
    pa_threaded_mainloop_stop(mainLoop);
  }

  if (pulseCtx) {
    pa_context_disconnect(pulseCtx);
    pa_context_unref(pulseCtx);
  }

  if (mainLoop) {
    pa_threaded_mainloop_free(mainLoop);
  }
}

ResponseMessage PulseAudioManager::handle(const PARequest &req) {
  ResponseMessage resp;

  std::visit(
      [&](auto &reqMsg) {
        using T = std::decay_t<decltype(reqMsg)>;

        if constexpr (std::is_same_v<T, PASetVolumeRequest>) {
          resp = setVolume(reqMsg);
        } else if constexpr (std::is_same_v<T, PAToggleMuteRequest>) {
          resp = toggleMute(reqMsg);
        } else if constexpr (std::is_same_v<T, PAUpdateDefDeviceRequest>) {
          resp = updateDefDevice(reqMsg);
        }
      },
      req);

  return resp;
}
