#pragma once

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
#include "../../../logging/manager.hpp"

struct Workspace {
  long int id;
  unsigned int monitorId;
  std::string name, monitor;
  bool fullScreen;
};

struct WSListenerData {
    GtkWidget* wsBox; 
    GtkWidget* spWSBox;
};

class HyprWSManager {
  std::string sockPath;
  int evtSockfd;
  std::thread eventListenerThread;
  Json::CharReaderBuilder jsonReader;
  LoggingManager *logger;
  bool failed;
  
  std::vector<std::pair<std::function<void(HyprWSManager *wsInstance, GtkWidget* wsBox, GtkWidget* spWSBox)>, WSListenerData>> listeners;

  int getPath();

  long parseWorkspaceId(std::string_view);
  Json::Value executeQuery(const std::string &, std::string &);

public:
  long activeWorkspaceId;
  std::map<long, Workspace> workspaces;
  
  HyprWSManager(LoggingManager *logMgr);
  ~HyprWSManager();
  
  void subscribe(std::function<void(HyprWSManager* wsInstance, GtkWidget* wsBox, GtkWidget* spWSBox)> updateFunc, GtkWidget* wsBox, GtkWidget* spWSBox);
  void liveEventListener();
  int GetWorkspaces();
  static int SwitchToWS(HyprWSManager* wsInstance, int wsId);
  // 
  static int SwitchSPWS(HyprWSManager* wsInstance, int wsId, std::string name);

};