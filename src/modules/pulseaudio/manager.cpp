#include "manager.hpp"
#include <pulse/subscribe.h>
#include <pulse/thread-mainloop.h>

#define TAG "PlayingNow"

int PulseAudioManager::Init(LoggingManager *logMgr) {
  logger = logMgr;

  pa_threaded_mainloop *mainLoop = pa_threaded_mainloop_new();
  if (mainLoop == nullptr) {
    logger->LogError(TAG, "Failed to Get Pulseaudio Main Loop.");
    return 1;
  }

  pa_mainloop_api *mainLoopAPI = pa_threaded_mainloop_get_api(mainLoop);
  pulseContext = pa_context_new(mainLoopAPI, "hyprsic");
  if (!pulseContext) {
    logger->LogError(TAG, "Failed to Create a Pulseaudio Context.");
    return 1;
  }

  pa_context_set_state_callback(pulseContext, contextStateHandler, this);

  if (pa_context_connect(pulseContext, nullptr, PA_CONTEXT_NOFAIL, nullptr) <
      0) {
    logger->LogError(TAG, "Failed to Connect the Pulseaudio Context.");
    return 1;
  }

  if (pa_threaded_mainloop_start(mainLoop) < 0) {
    logger->LogError(TAG, "Failed to Start the Pulseaudio Context.");
    return 1;
  }

  return 0;
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
        (enum pa_subscription_mask)(
            PA_SUBSCRIPTION_EVENT_SINK_INPUT |
            PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT | PA_SUBSCRIPTION_EVENT_SERVER |
            PA_SUBSCRIPTION_EVENT_SOURCE | PA_SUBSCRIPTION_EVENT_SINK),
        nullptr, nullptr);
    self->logger->LogInfo(TAG, "Successfully Subscribed to Pulseaudio Events.");
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

  playing->data.server = info->server_name;
  playing->data.source = info->default_source_name;
  playing->data.sink = info->default_sink_name;
}

void PulseAudioManager::handleStateChanges(
    pa_context *pulseCtx, const pa_subscription_event_type eventType,
    unsigned int idx, void *data) {
  unsigned int facility = eventType & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;

  switch (facility) {

  case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
    pa_context_get_sink_input_info(pulseCtx, idx, outputInfoCallBack, data);
    break;

    //  case PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT:
    //  pa_context_get_source_output_info(pulseCtx, idx, inputInfoCallBack,
    //  data);
    //    break;

  case PA_SUBSCRIPTION_EVENT_SERVER:
  case PA_SUBSCRIPTION_EVENT_SOURCE:
  case PA_SUBSCRIPTION_EVENT_SINK:
    pa_context_get_server_info(pulseCtx, serverInfoCallBack, data);
    break;
  }
}

void PulseAudioManager::outputInfoCallBack(pa_context *pulseCtx,
                                           const pa_sink_input_info *info,
                                           int eol, void *data) {
  if (info == nullptr)
    return;
  pa_context_get_client_info(pulseCtx, info->client, clientInfoCallBack, data);

  PulseAudioManager *playing = (PulseAudioManager *)data;
  playing->data.out = info->name;
}

void PulseAudioManager::clientInfoCallBack(pa_context *pulseCtx,
                                           const pa_client_info *info, int eol,
                                           void *data) {
  if (info == nullptr)
    return;

  PulseAudioManager *playing = (PulseAudioManager *)data;
  playing->data.client = info->name;
}

void PulseAudioManager::inputInfoCallBack(pa_context *pulseCtx,
                                          const pa_source_output_info *info,
                                          int eol, void *data) {
  if (info == nullptr)
    return;

  PulseAudioManager *self = static_cast<PulseAudioManager *>(data);
  std::string msg = "Input Name: ";
  msg += info->name;
  self->logger->LogInfo(TAG, msg);
}
