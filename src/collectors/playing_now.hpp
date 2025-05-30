#pragma once
#include "string"
#include <pulse/context.h>
#include <pulse/introspect.h>

struct PulseAudioData {
  std::string server, source, sink, out, client, in;
};

class PlayingNow {
private:
  pa_context *pulseContext;
  static void contextStateHandler(pa_context *, void *);
  static void handleStateChanges(pa_context *, const pa_subscription_event_type,
                                 unsigned int, void *);

  static void serverInfoCallBack(pa_context *, const pa_server_info *, void *);
  static void outputInfoCallBack(pa_context *, const pa_sink_input_info *, int,
                                 void *);
  static void inputInfoCallBack(pa_context *, const pa_source_output_info *,
                                int, void *);
  static void clientInfoCallBack(pa_context *, const pa_client_info *, int,
                                 void *);

public:
  PulseAudioData data;
  int Init();
};
