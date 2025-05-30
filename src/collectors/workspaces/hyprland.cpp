#include "hyprland.hpp"
#include "cstdlib"
#include "cstring"
#include "iostream"
#include "sstream"
#include "sys/poll.h"
#include "sys/socket.h"
#include "sys/un.h"
#include "thread"
#include "unistd.h"
#include "unordered_map"

int HyprWorkspaces::Init() {
  if (getPath()) {
    return 1;
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
    std::cerr << "[Init Error] Unable to Establish Connection with Hyprland "
                 "Socket UNIX Socket"
              << std::endl;
    return 1;
  }

  GetWorkspaces();
  liveEventListener();

  return 0;
}

HyprWorkspaces::~HyprWorkspaces() { close(evtSockfd); }

int HyprWorkspaces::getPath() {
  char *runtimeDir = std::getenv("XDG_RUNTIME_DIR");
  if (runtimeDir == nullptr) {
    std::cerr << "XDG_RUNTIME_DIR is not set" << std::endl;
    return 1;
  }

  char *HIS = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
  if (HIS == nullptr) {
    std::cerr << "HYPRLAND_INSTANCE_SIGNATURE is not set" << std::endl;
    return 1;
  }

  sockPath.append(runtimeDir).append("/hypr/").append(HIS).append("/");
  return 0;
}

void HyprWorkspaces::liveEventListener() {
  std::thread([&]() {
    char buffer[1024];

    struct pollfd pollConfig[] = {{evtSockfd, POLLIN, POLLRDBAND}};

    while (true) {
      if (poll(pollConfig, 1, -1) > 0) {
        // Reading The Event Info
        if (read(evtSockfd, buffer, 1024) <= 0) {
          std::cerr << "Error while reading the event info" << std::endl;
          continue;
        }
        // std::cout<<"\n\nEvent Info:\n"<<buffer<<"\n\n"<<std::endl;

        // Update the String Length Too If Event name is being updated
        if (std::strncmp(buffer, "createworkspace", 15) == 0) {
          char *ptr = std::strstr(buffer, "createworkspacev2>>");
          std::cout << "Workspace Created: " << parseWorkspaceId(ptr + 19)
                    << std::endl;

        } else if (std::strncmp(buffer, "destroyworkspace", 16) == 0) {
          char *ptr = std::strstr(buffer, "destroyworkspacev2>>");
          std::cout << "Workspace Deleted: " << parseWorkspaceId(ptr + 20)
                    << std::endl;

        } else if (std::strncmp(buffer, "workspace", 9) == 0) {
          char *ptr = std::strstr(buffer, "workspacev2>>");
          std::cout << "Active Workspace Changed: "
                    << parseWorkspaceId(ptr + 13) << std::endl;
        }
      }
    }
  }).detach();
}

int HyprWorkspaces::parseWorkspaceId(char *stPoint) {
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

Json::Value HyprWorkspaces::executeQuery(const std::string &msg,
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
    std::cerr << "[IPC Connection Error] Unable to Establish Connection with "
                 "hyprctl UNIX Socket"
              << std::endl;
    return -3;
  }

  if (write(workSockfd, msg.c_str(), msg.size()) == -1) {
    err = "Failed to Send Query";
    return -2;
  }
  if (read(workSockfd, buffer, sizeof(buffer)) <= 0) {
    err = "Failed to Read Query Response";
    return -2;
  }

  // Closing the Connection
  if (close(workSockfd) == -1) {
    std::cerr << "[IPC Connection Error] Unable to Close Connection with "
                 "hyprctl UNIX Socket"
              << std::endl;
    return -3;
  }

  // Parsing the json response
  Json::Value root;
  std::istringstream jsonStream(buffer);
  if (!Json::parseFromStream(jsonReader, jsonStream, &root, &err)) {
    return -1;
  }
  return root;
}

int HyprWorkspaces::GetActiveWorkspace() {
  std::string err;
  Json::Value workspaceJson = executeQuery("j/activeworkspace", err);
  if (workspaceJson == -1) {
    std::cerr << "[Error] Failed to parse Active Workspace Query Response ("
              << err << ")" << std::endl;
    return 1;
  } else if (workspaceJson == -2) {
    std::cerr << "[Error] In Active Workspace Query (" << err << ")"
              << std::endl;
    return 1;
  }

  activeWorkspaceId = workspaceJson["id"].asUInt();
  return 0;
}

Workspace HyprWorkspaces::GetActiveWorkspaceInfo() {
  auto ele = workspaces.find(activeWorkspaceId);
  if (ele == workspaces.end()) {
    return Workspace{};
  } else {
    return ele->second;
  }
}

int HyprWorkspaces::GetWorkspaces() {
  std::string err;
  Json::Value workspacesJson = executeQuery("j/workspaces", err);
  if (workspacesJson == -1) {
    std::cerr << "[Error] Failed to parse Workspaces Query Response (" << err
              << ")" << std::endl;
    return 1;
  } else if (workspacesJson == -2) {
    std::cerr << "[Error] In Workspaces Query (" << err << ")" << std::endl;
    return 1;
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

  return 0;
}
