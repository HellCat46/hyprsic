#pragma once

#include "cstdlib"
#include "cstring"
#include "gtk/gtk.h"
#include "services/header/comm_types.hpp"
#include "services/header/logging.hpp"
#include "string"
#include <cstdint>
#include <json/reader.h>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <thread>
#include <variant>

struct Workspace {
  long int id;
  unsigned int monitorId;
  std::string name, monitor;
  bool fullScreen;
};

struct WSListenerData {
  GtkWidget *wsBox;
  GtkWidget *spWSBox;
  unsigned char windowId;
};

struct HyprSwitchWSRequest {
  unsigned int wsId;

  uint64_t correlationId;
};

struct HyprMoveWSRequest {
  unsigned char monitorId;
  bool forw;

  uint64_t correlationId;
};

struct HyprSwitchSPWSRequest {
  unsigned int wsId;
  std::string name;

  uint64_t correlationId;
};

using HyprRequest =
    std::variant<HyprSwitchSPWSRequest, HyprSwitchWSRequest, HyprMoveWSRequest>;
class HyprWSManager {
  std::string sockPath;
  int evtSockfd;
  std::thread eventListenerThread;
  Json::CharReaderBuilder jsonReader;
  LoggingManager *logger;
  bool failed;

  // TODO: Replace it with CommunicationBus.... Not sure how tho... 
  // std::vector<
  //     std::pair<std::function<void(CommunicationBus *commBus,HyprWSManager *wsInstance, GtkWidget *wsBox,
  //                                  GtkWidget *spWSBox, unsigned char windowId)>,
  //               WSListenerData>>
  //     listeners;

  int getPath();

  long parseWorkspaceId(std::string_view);
  Json::Value executeQuery(const std::string &, std::string &);

  ResponseMessage SwitchToWS(const HyprSwitchWSRequest& req);
  ResponseMessage MoveToWS(const HyprMoveWSRequest& req);
  ResponseMessage SwitchSPWS(const HyprSwitchSPWSRequest& req);

public:
  unsigned int activeWorkspaceId;
  std::map<long, Workspace> workspaces;
  std::map<std::string, unsigned int> monitors;

  HyprWSManager(LoggingManager *logMgr);
  ~HyprWSManager();

  // void subscribe(std::function<void(const HyprWSManager *wsInstance, GtkWidget *wsBox,
  //                                   GtkWidget *spWSBox, unsigned char windowId)>
  //                    updateFunc,
  //                GtkWidget *wsBox, GtkWidget *spWSBox, unsigned char windowId);

  void liveEventListener();
  int GetWorkspaces();
  int GetMonitors();

  ResponseMessage handle(const HyprRequest &msg);
};
