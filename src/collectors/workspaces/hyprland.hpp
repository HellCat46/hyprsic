#pragma once

#include "cstdlib"
#include "cstring"
#include "gtk/gtk.h"
#include "string"
#include <functional>
#include <json/reader.h>

struct Workspace {
  unsigned int id, monitorId;
  std::string name, monitor;
  bool fullScreen;
};

class HyprWorkspaces {
  std::string sockPath;
  int evtSockfd;
  Json::CharReaderBuilder jsonReader;

  int getPath();
  
  int parseWorkspaceId(char *);
  Json::Value executeQuery(const std::string &, std::string &);

public:
  int activeWorkspaceId;
  std::map<unsigned int, Workspace> workspaces;
  int GetWorkspaces();
  int Init();
  void liveEventListener(std::function<void(HyprWorkspaces* wsInstance, GtkWidget* workspaceBox)> updateFunc, GtkWidget* workspaceBox);
  static int SwitchToWorkspace(HyprWorkspaces* wsInstance, int wsId);

  ~HyprWorkspaces();
};
