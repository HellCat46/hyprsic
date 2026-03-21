#pragma once

#include "cstdlib"
#include "cstring"
#include "gtk/gtk.h"
#include "services/header/comm_types.hpp"
#include "services/header/logging.hpp"
#include "string"
#include <functional>
#include <json/reader.h>
#include <string>
#include <string_view>
#include <thread>
#include <variant>
#include <vector>

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
  int wsId;

  long int correlationId;
};

struct HyprMoveWSRequest {
  int wsId;
  unsigned char monitorId;
  bool forw;

  long int correlationId;
};

struct HyprSwitchSPWSRequest {
  int wsId;
  std::string name;

  long int correlationId;
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

  std::vector<
      std::pair<std::function<void(HyprWSManager *wsInstance, GtkWidget *wsBox,
                                   GtkWidget *spWSBox, unsigned char windowId)>,
                WSListenerData>>
      listeners;

  int getPath();

  long parseWorkspaceId(std::string_view);
  Json::Value executeQuery(const std::string &, std::string &);

  ResponseMessage SwitchToWS(HyprSwitchWSRequest req);
  ResponseMessage MoveToWS(HyprMoveWSRequest req);
  ResponseMessage SwitchSPWS(HyprSwitchSPWSRequest req);

public:
  long activeWorkspaceId;
  std::map<long, Workspace> workspaces;
  std::map<std::string, unsigned int> monitors;

  HyprWSManager(LoggingManager *logMgr);
  ~HyprWSManager();

  void subscribe(std::function<void(HyprWSManager *wsInstance, GtkWidget *wsBox,
                                    GtkWidget *spWSBox, unsigned char windowId)>
                     updateFunc,
                 GtkWidget *wsBox, GtkWidget *spWSBox, unsigned char windowId);

  void liveEventListener();
  int GetWorkspaces();
  int GetMonitors();

  ResponseMessage handle(const HyprRequest &msg);
};
