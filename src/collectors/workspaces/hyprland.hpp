#pragma once

#include "cstdlib"
#include "cstring"
#include "string"
#include "unordered_map"
#include <json/reader.h>

struct Workspace {
  unsigned int id, monitorId;
  std::string name, monitor;
  bool fullScreen;
};

class HyprWorkspaces {
  std::string sockPath;
  int evtSockfd, activeWorkspaceId;
  std::unordered_map<unsigned int, Workspace> workspaces;
  Json::CharReaderBuilder jsonReader;

  int getPath();
  void liveEventListener();
  int parseWorkspaceId(char *);
  Json::Value executeQuery(const std::string &, std::string &);

public:
  int GetActiveWorkspace();
  Workspace GetActiveWorkspaceInfo();
  int GetWorkspaces();
  int Init();

  ~HyprWorkspaces();
};
