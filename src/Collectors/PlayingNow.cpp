#include "iostream"
#include "pulse/pulseaudio.h"

struct PulseAudioData {
  std::string server, source, sink, out, client, in;
};

class PlayingNow {
  private:
  pa_context *pulseContext;
  static void contextStateHandler(pa_context *, void *);
  static void handleStateChanges(pa_context*, const pa_subscription_event_type, unsigned int, void*);

  static void serverInfoCallBack(pa_context*, const pa_server_info *, void *);
  static void outputInfoCallBack(pa_context*, const pa_sink_input_info*, int, void*);
  static void inputInfoCallBack(pa_context*, const pa_source_output_info*, int, void*);
  static void clientInfoCallBack(pa_context*, const pa_client_info*, int, void*);

  public:
    PulseAudioData data;
    int Init(){
      pa_threaded_mainloop *mainLoop = pa_threaded_mainloop_new();
      if(mainLoop == nullptr){
        std::cerr<<"[Init Error] Failed to Get Pulseaudio Main Loop."<<std::endl;
        return 1;
      }

      pa_mainloop_api *mainLoopAPI = pa_threaded_mainloop_get_api(mainLoop);
      pulseContext = pa_context_new(mainLoopAPI, "hyprsic");
      if(!pulseContext){
        std::cerr<<"[Init Error] Failed to Create a Pulseaudio Context."<<std::endl;
        return 1;
      }

      pa_context_set_state_callback(pulseContext, contextStateHandler, this);


      if (pa_context_connect(pulseContext, nullptr, PA_CONTEXT_NOFAIL, nullptr) < 0) {
	    std::cerr<<"[Init Error] Failed to Connect the Pulseaudio Context."<<std::endl;
    	return 1;
	  }

      if (pa_threaded_mainloop_start(mainLoop) < 0) {
        std::cerr<<"[Init Error] Failed to Start the Pulseaudio Context."<<std::endl;
        return 1;
      }

      return 0;
    }
};

void PlayingNow::contextStateHandler(pa_context *pulseCtx, void *data){
  //std::cout<<pa_context_get_state(pulseCtx)<<std::endl;

  switch(pa_context_get_state(pulseCtx)){
    case PA_CONTEXT_READY:
      //pa_context_get_sink_info_list(pulseCtx, outputInfoCallBack, data);
      pa_context_get_server_info(pulseCtx, serverInfoCallBack,data);

      pa_context_set_subscribe_callback(pulseCtx, handleStateChanges, data);
      pa_context_subscribe(pulseCtx,
                           (enum pa_subscription_mask)(PA_SUBSCRIPTION_EVENT_SINK_INPUT |
                           PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT |
                           PA_SUBSCRIPTION_EVENT_SERVER |
                           PA_SUBSCRIPTION_EVENT_SOURCE |
                           PA_SUBSCRIPTION_EVENT_SINK), nullptr, nullptr);

      break;
    case PA_CONTEXT_TERMINATED: std::cout<<"Connection Terminated"<<std::endl;
      break;
    case PA_CONTEXT_FAILED: std::cout<<"Connection Failed"<<std::endl;
      break;
  }
}

void PlayingNow::serverInfoCallBack(pa_context *pulseCtx, const pa_server_info *info, void *data){
    class PlayingNow *playing = (PlayingNow *) data;

    playing->data.server = info->server_name;
    playing->data.source = info->default_source_name;
    playing->data.sink = info->default_sink_name;
}

void PlayingNow::handleStateChanges(pa_context *pulseCtx, const  pa_subscription_event_type eventType, unsigned int idx, void *data){
  unsigned int facility = eventType & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;

  switch(facility){

    case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
      pa_context_get_sink_input_info(pulseCtx, idx, outputInfoCallBack, data);
      break;

//  case PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT: pa_context_get_source_output_info(pulseCtx, idx, inputInfoCallBack, data);
//    break;

    case PA_SUBSCRIPTION_EVENT_SERVER:
    case PA_SUBSCRIPTION_EVENT_SOURCE:
    case PA_SUBSCRIPTION_EVENT_SINK:
      pa_context_get_server_info(pulseCtx, serverInfoCallBack,data);
      break;
  }
}

void PlayingNow::outputInfoCallBack(pa_context *pulseCtx, const pa_sink_input_info *info, int eol, void *data){
    if(info == nullptr) return;
    pa_context_get_client_info(pulseCtx, info->client, clientInfoCallBack, data);

    PlayingNow *playing = (PlayingNow*) data;
    playing->data.out = info->name;
}

void PlayingNow::clientInfoCallBack(pa_context *pulseCtx, const pa_client_info *info, int eol, void *data){
  if(info == nullptr) return;

  PlayingNow *playing = (PlayingNow*) data;
  playing->data.client = info->name;
}

void PlayingNow::inputInfoCallBack(pa_context *pulseCtx, const pa_source_output_info *info, int eol, void *data) {
  if(info == nullptr) return;

  std::cout<<"Input Name: "<<info->name<<std::endl;
}
