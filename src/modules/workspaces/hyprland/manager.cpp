#include "manager.hpp"
#include "cstdlib"
#include "sstream"
#include "sys/poll.h"
#include "sys/socket.h"
#include "sys/un.h"
#include "thread"
#include "unistd.h"
#include <cstddef>
#include <string>
#include <string_view>

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
        memset(buffer, 0, 1024);

        // Reading The Event Info
        if (read(evtSockfd, buffer, 1024) <= 0) {
          logger->LogError(TAG, "Error while reading the event info");
          continue;
        }

        chngMade = false;
        std::string_view buffView(buffer);

        // Update the String Length Too If Event name is being updated
        if (buffView.starts_with("createworkspace")) {
          size_t pos = buffView.find("createworkspacev2>>");

          if (pos != std::string_view::npos) {
            buffView = buffView.substr(pos + 19);

            long wsId = parseWorkspaceId(buffView);
            if (wsId == 0) {
              logger->LogError(TAG, "Failed to Parse Workspace ID from Event "
                                    "Info - Create Workspace");
              continue;
            }

            chngMade = true;
            logger->LogInfo(TAG, "Workspace Created: " + std::to_string(wsId));
          }
        }

        if (buffView.starts_with("workspace")) {
          size_t pos = buffView.find("workspacev2>>");
          if (pos != std::string_view::npos) {
            buffView = buffView.substr(pos + 13);

            long wsId = parseWorkspaceId(buffView);
            if (wsId == 0) {
              logger->LogError(TAG, "Failed to Parse Workspace ID from Event "
                                    "Info - Switch Workspace");
              continue;
            }

            chngMade = true;
            activeWorkspaceId = wsId;
            // logger->LogInfo(TAG,
            //                 "Active Workspace Changed: " +
            //                 std::to_string(wsId));
          }
        }

        if (buffView.starts_with("focusedmon")) {
          size_t pos = buffView.find("focusedmonv2>>");
          if (pos != std::string_view::npos) {
            buffView = buffView.substr(pos + 14);

            pos = buffView.find(",");
            if (pos != std::string_view::npos) {
              buffView = buffView.substr(pos + 1);
              long wsId = parseWorkspaceId(buffView);
              if (wsId == 0) {
                logger->LogError(TAG, "Failed to Parse Workspace ID from Event "
                                      "Info - Focused Monitor Changed");
                continue;
              }

              chngMade = true;
              activeWorkspaceId = wsId;
              // logger->LogInfo(TAG,
              //                 "Focused Monitor Changed: " +
              //                 std::to_string(wsId));
            }
          }
        }

        if (buffView.starts_with("destroyworkspace")) {
          size_t pos = buffView.find("destroyworkspacev2>>");
          if (pos != std::string_view::npos) {
            buffView = buffView.substr(pos + 20);

            int wsId = parseWorkspaceId(buffView);
            if (wsId == 0) {
              logger->LogError(TAG, "Failed to Parse Workspace ID from Event "
                                    "Info - Destroy Workspace");
              continue;
            }

            chngMade = true;
            workspaces.erase(wsId);
            logger->LogInfo(TAG, "Workspace Deleted: " + std::to_string(wsId));
          }
        }

        if (chngMade) {
          for (auto &listener : listeners) {
            listener.first(this, listener.second.wsBox,
                           listener.second.spWSBox);
          }
        }
      }
    }
  });
}

long HyprWSManager::parseWorkspaceId(std::string_view stPoint) {
  if (stPoint.empty()) {
    return 0;
  }

  long id = 0;
  bool neg = false;

  for (int idx = 0; stPoint[idx] != '\n' && stPoint[idx] != '\0' &&
                    stPoint[idx] != ',' && idx < stPoint.size();
       idx++) {
    if (stPoint[idx] >= 48 && stPoint[idx] <= 57) {
      id = (id * 10) + (stPoint[idx] - 48);
    } else if (stPoint[idx] == '-') {
      neg = true;
    }
  }

  if (neg) {
    id = -id;
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

int HyprWSManager::SwitchToWS(HyprWSManager *wsInstance, int wsId) {
  if (wsInstance->workspaces.find(wsId) == wsInstance->workspaces.end()) {
    wsInstance->logger->LogError(TAG, "Workspace Not Found");
    return 1;
  }

  std::string err;
  if (wsInstance->executeQuery("s/dispatch workspace " + std::to_string(wsId),
                               err) == -2) {

    wsInstance->logger->LogError(TAG, "Failed to Switch to Workspace: " + err);
    return 1;
  }
  return 0;
}

int HyprWSManager::SwitchSPWS(HyprWSManager *wsInstance, int wsId,
                              std::string name) {
  if (wsInstance->workspaces.find(wsId) == wsInstance->workspaces.end()) {
    wsInstance->logger->LogError(TAG, "Workspace Not Found");
    return 1;
  }

  std::string err;
  if (wsInstance->executeQuery("s/dispatch togglespecialworkspace " + name,
                               err) == -2) {

    wsInstance->logger->LogError(
        TAG, "Failed to Switch to Special Workspace: " + err);
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
        workspace["id"].asLargestInt(),      workspace["monitorID"].asUInt(),
        workspace["name"].asString(),        workspace["monitor"].asString(),
        workspace["hasfullscreen"].asBool(),
    };

    if (wSpace.id < 0) {
      auto pos = wSpace.name.find(':');
      if (pos != std::string::npos) {
        wSpace.name = wSpace.name.substr(pos + 1);
      }
    }

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
    std::function<void(HyprWSManager *wsInstance, GtkWidget *wsBox,
                       GtkWidget *spWSBox)>
        updateFunc,
    GtkWidget *wsBox, GtkWidget *spWSBox) {
  listeners.push_back({updateFunc, WSListenerData{wsBox, spWSBox}});
}
