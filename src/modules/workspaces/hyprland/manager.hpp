#pragma once

#include "../../../logging/manager.hpp"
#include "cstdlib"
#include "cstring"
#include "gtk/gtk.h"
#include "string"
#include <functional>
#include <json/reader.h>
#include <string>
#include <string_view>
#include <thread>
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

public:
  long activeWorkspaceId;
  std::map<long, Workspace> workspaces;

  HyprWSManager(LoggingManager *logMgr);
  ~HyprWSManager();

  void subscribe(std::function<void(HyprWSManager *wsInstance, GtkWidget *wsBox,
                                    GtkWidget *spWSBox, unsigned char windowId)>
                     updateFunc,
                 GtkWidget *wsBox, GtkWidget *spWSBox, unsigned char windowId);
  
  void liveEventListener();
  int GetWorkspaces();
  
  int SwitchToWS(int wsId);
  int MoveToWS(int wsId, unsigned char monitorId, bool forw);
  int SwitchSPWS(int wsId, std::string name);
};
