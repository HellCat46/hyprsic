#include "manager.hpp"
#include "cstdlib"
#include "cstring"
#include "sstream"
#include "sys/poll.h"
#include "sys/socket.h"
#include "sys/un.h"
#include "thread"
#include "unistd.h"
#include <string>

#define TAG "HyprWorkspaces"

HyprWSManager::HyprWSManager(LoggingManager *logMgr) : logger(logMgr) {
  failed = false;

  if (getPath()) {
    failed = true;
    return;
  }

  sockaddr_un addr;
  addr.sun_family = AF_UNIX;

  evtSockfd = socket(AF_UNIX, SOCK_STREAM, 0);

  // Establish Connection With Hyprland's UNIX Socket for Listening to Client
  // events
  sockPath += ".socket2.sock";
  strcpy(addr.sun_path, sockPath.c_str());
  sockPath = sockPath.substr(0, sockPath.rfind('/') + 1);
  if (connect(evtSockfd, (sockaddr *)&addr, sizeof(addr)) == -1) {
    logger->LogError(
        TAG, "Unable to Establish Connection with Hyprland Socket UNIX Socket");
    failed = true;
    return;
  }

  GetWorkspaces();

  return;
}

HyprWSManager::~HyprWSManager() { close(evtSockfd); }

int HyprWSManager::getPath() {
  char *runtimeDir = std::getenv("XDG_RUNTIME_DIR");
  if (runtimeDir == nullptr) {
    logger->LogError(TAG, "XDG_RUNTIME_DIR is not set");
    return -1;
  }

  char *HIS = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
  if (HIS == nullptr) {
    logger->LogError(TAG, "HYPRLAND_INSTANCE_SIGNATURE is not set");
    return -2;
  }

  sockPath.append(runtimeDir).append("/hypr/").append(HIS).append("/");
  return 0;
}

void HyprWSManager::liveEventListener() {

  eventListenerThread = std::thread([this]() {
    char buffer[1024];

    struct pollfd pollConfig[] = {{evtSockfd, POLLIN, POLLRDBAND}};
    bool chngMade = false;

    while (true) {
      if (poll(pollConfig, 1, -1) > 0) {
        // Reading The Event Info
        if (read(evtSockfd, buffer, 1024) <= 0) {
          logger->LogError(TAG, "Error while reading the event info");
          continue;
        }

        GetWorkspaces();
        chngMade = false;

        // Update the String Length Too If Event name is being updated
        if (std::strncmp(buffer, "createworkspace", 15) == 0) {
          char *ptr = std::strstr(buffer, "createworkspacev2>>");
          int wsId = parseWorkspaceId(ptr + 19);
          chngMade = true;

          logger->LogInfo(TAG, "Workspace Created: " + std::to_string(wsId));

        } else if (std::strncmp(buffer, "workspace", 9) == 0) {
          char *ptr = std::strstr(buffer, "workspacev2>>");
          int wsId = parseWorkspaceId(ptr + 13);
          chngMade = true;

          activeWorkspaceId = wsId;
          // logger->LogInfo(TAG,
          //                 "Active Workspace Changed: " +
          //                 std::to_string(wsId));
        } else if(!std::strncmp(buffer, "focusedmon", 10)){
          char *ptr = std::strstr(buffer, "focusedmonv2>>");
          int wsId = parseWorkspaceId(ptr + 14);
          chngMade = true;

          activeWorkspaceId = wsId;
        }else {

          if (std::strstr(buffer, "destroyworkspace") != nullptr) {
            char *ptr = std::strstr(buffer, "destroyworkspacev2>>");
            int wsId = parseWorkspaceId(ptr + 20);
            chngMade = true;

            workspaces.erase(wsId);
            logger->LogInfo(TAG, "Workspace Deleted: " + std::to_string(wsId));
          }
        }

        if (chngMade) {
          for (auto &listener : listeners) {
            listener.first(this, listener.second);
          }
        }
      }
    }
  });
}

int HyprWSManager::parseWorkspaceId(char *stPoint) {
  int id = 0;

  for (int idx = 0;
       stPoint[idx] != '\n' && stPoint[idx] != '\0' && stPoint[idx] != ',';
       idx++) {
    if (stPoint[idx] >= 48 && stPoint[idx] <= 57) {
      id = (id * 10) + (stPoint[idx] - 48);
    }
  }
  return id;
}

Json::Value HyprWSManager::executeQuery(const std::string &msg,
                                        std::string &err) {
  int workSockfd;
  char buffer[8192];

  workSockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  sockaddr_un addr;

  addr.sun_family = AF_UNIX;

  // Establish Connection With Hyprland's UNIX Socket for Performing Workspace
  // Related Action
  sockPath += ".socket.sock";
  strcpy(addr.sun_path, sockPath.c_str());
  sockPath = sockPath.substr(0, sockPath.rfind('/') + 1);
  if (connect(workSockfd, (sockaddr *)&addr, sizeof(addr)) == -1) {
    logger->LogError(TAG,
                     "Unable to Establish Connection with hyprctl UNIX Socket");
    return -1;
  }

  if (write(workSockfd, msg.c_str(), msg.size()) == -1) {
    err = "Failed to Send Query";
    return -2;
  }
  if (read(workSockfd, buffer, sizeof(buffer)) <= 0) {
    err = "Failed to Read Query Response";
    return -3;
  }

  // Closing the Connection
  if (close(workSockfd) == -1) {
    logger->LogError(TAG,
                     "Unable to Close Connection with hyprctl UNIX Socket");
    return -4;
  }

  // Parsing the json response
  Json::Value root;
  std::istringstream jsonStream(buffer);
  if (!Json::parseFromStream(jsonReader, jsonStream, &root, &err)) {
    return -1;
  }
  return root;
}

int HyprWSManager::SwitchToWorkspace(HyprWSManager *wsInstance, int wsId) {
  if (wsInstance->workspaces.find(wsId) == wsInstance->workspaces.end()) {
    wsInstance->logger->LogError(TAG, "Workspace Not Found");
    return 1;
  }

  std::string err;
  if (wsInstance->executeQuery("s/dispatch workspace " + std::to_string(wsId),
                               err) == -2) {
    std::string errMsg = "Failed to Switch to Workspace: ";
    errMsg += err;
    wsInstance->logger->LogError(TAG, errMsg);
    return 1;
  }
  return 0;
}

int HyprWSManager::GetWorkspaces() {
  std::string err;
  Json::Value workspacesJson = executeQuery("j/workspaces", err);
  if (workspacesJson == -1) {
    std::string errMsg = "Failed to parse Workspaces Query Response (";
    errMsg += err;
    errMsg += ")";
    logger->LogError(TAG, errMsg);
    return -1;
  } else if (workspacesJson == -2) {
    std::string errMsg = "In Workspaces Query (";
    errMsg += err;
    errMsg += ")";
    logger->LogError(TAG, errMsg);
    return -1;
  }

  for (Json::Value workspace : workspacesJson) {
    Workspace wSpace = {
        workspace["id"].asUInt(),
        workspace["monitorID"].asUInt(),
        workspace["name"].asString(),
        workspace["monitor"].asString(),
        workspace["hasfullscreen"].asBool(),
    };
    workspaces.insert({wSpace.id, wSpace});
  }

  Json::Value activeWsJson = executeQuery("j/activeworkspace", err);
  if (activeWsJson == -1) {
    std::string errMsg = "Failed to parse Active Workspace Query Response (";
    errMsg += err;
    errMsg += ")";
    logger->LogError(TAG, errMsg);
    return -1;
  } else if (activeWsJson == -2) {
    std::string errMsg = "In Active Workspace Query (";
    errMsg += err;
    errMsg += ")";
    logger->LogError(TAG, errMsg);
    return -1;
  }
  activeWorkspaceId = activeWsJson["id"].asUInt();
  return 0;
}

void HyprWSManager::subscribe(
    std::function<void(HyprWSManager *wsInstance, GtkWidget *workspaceBox)>
        updateFunc,
    GtkWidget *workspaceBox) {
  listeners.push_back({updateFunc, workspaceBox});
}
