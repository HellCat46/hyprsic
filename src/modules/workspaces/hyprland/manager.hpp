#pragma once

#include "cstdlib"
#include "cstring"
#include "gtk/gtk.h"
#include "string"
#include <functional>
#include <json/reader.h>
#include <thread>
#include <vector>
#include "../../../logging/manager.hpp"

struct Workspace {
  unsigned int id, monitorId;
  std::string name, monitor;
  bool fullScreen;
};

class HyprWSManager {
  std::string sockPath;
  int evtSockfd;
  std::thread eventListenerThread;
  Json::CharReaderBuilder jsonReader;
  LoggingManager *logger;
  bool failed;
  
  std::vector<std::pair<std::function<void(HyprWSManager *wsInstance, GtkWidget *workspaceBox)>, GtkWidget*>> listeners;

  int getPath();

  int parseWorkspaceId(char *);
  Json::Value executeQuery(const std::string &, std::string &);

public:
  int activeWorkspaceId;
  std::map<unsigned int, Workspace> workspaces;
  
  HyprWSManager(LoggingManager *logMgr);
  ~HyprWSManager();
  
  void subscribe(std::function<void(HyprWSManager* wsInstance, GtkWidget* workspaceBox)> updateFunc, GtkWidget* workspaceBox);
  void liveEventListener();
  int GetWorkspaces();
  static int SwitchToWorkspace(HyprWSManager* wsInstance, int wsId);

};