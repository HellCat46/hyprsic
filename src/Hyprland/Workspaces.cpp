#include "iostream"
#include "sys/socket.h"
#include "sys/un.h"
#include "json/json.h"
#include "vector"
#include "unistd.h"
#include "cstring"
#include "cstdlib"
#include "sstream"
#include "unordered_map"
#include "thread"
#include "chrono"

struct Workspace {
  unsigned int id, monitorId;
  std::string name, monitor;
  bool fullScreen;
};

class HyprWorkspaces {
  private:
  std::string sockPath;
  int evtSockfd, activeWorkspaceId;;
  std::unordered_map<unsigned int, Workspace> workspaces;
  Json::CharReaderBuilder jsonReader;


  int getPath();
  void liveEventListener();
  Json::Value executeQuery(const std::string&, std::string&);


  public:
    int GetActiveWorkspace();
    Workspace GetActiveWorkspaceInfo();
    int GetWorkspaces();

    int Init(){
      if(getPath()){
        return 1;
      }

      sockaddr_un addr;
      addr.sun_family = AF_UNIX;

      evtSockfd = socket(AF_UNIX, SOCK_STREAM,0);

      // Establish Connection With Hyprland's UNIX Socket for Listening to Client events
      sockPath += ".socket2.sock";
      strcpy(addr.sun_path, sockPath.c_str());
      sockPath = sockPath.substr(0, sockPath.rfind('/')+1);
      if(connect(evtSockfd, (sockaddr*) &addr, sizeof(addr)) == -1){
        std::cerr<<"[Init Error] Unable to Establish Connection with Hyprland Socket UNIX Socket"<<std::endl;
        return 1;
      }

      GetWorkspaces();
      liveEventListener();

      return 0;
    }

  ~HyprWorkspaces(){
      close(evtSockfd);
  }
};

int HyprWorkspaces::getPath(){
  char* runtimeDir = std::getenv("XDG_RUNTIME_DIR");
  if(runtimeDir == nullptr){
    std::cerr<<"XDG_RUNTIME_DIR is not set"<<std::endl;
    return 1;
  }

  char* HIS = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
  if(HIS == nullptr){
    std::cerr<<"HYPRLAND_INSTANCE_SIGNATURE is not set"<<std::endl;
    return 1;
  }

  sockPath.append(runtimeDir).append("/hypr/").append(HIS).append("/");
  return 0;
}

void HyprWorkspaces::liveEventListener(){
  std::thread([&](){
    char buffer[1024];
    while(true){
      int res = read(evtSockfd, buffer, 1024);
      if(res > 0){
        //std::cout<<"\n\nEvent Info:\n"<<buffer<<"\n\n"<<std::endl;

        // Update the String Length Too If Event name is being updated
        if(std::strncmp(buffer, "createworkspace", 15) == 0){
          char *ptr = std::strstr(buffer, "createworkspacev2>>");

          std::cout<<"Workspace Created: ";
          for(int idx = 19; ptr[idx] != '\n' && ptr[idx] != '\0'; idx++){
            std::cout<<ptr[idx];
          }
          std::cout<<std::endl;

        }else if(std::strncmp(buffer, "destroyworkspace", 16) == 0){
          char *ptr = std::strstr(buffer, "destroyworkspacev2>>");

          std::cout<<"Workspace Deleted: ";
          for(int idx = 20; ptr[idx] != '\n' && ptr[idx] != '\0'; idx++){
            std::cout<<ptr[idx];
          }
          std::cout<<std::endl;
        }else if(std::strncmp(buffer, "workspace", 9) == 0){
          char *ptr = std::strstr(buffer, "workspacev2>>");

          std::cout<<"Active Workspace Changed: ";
          for(int idx = 13; ptr[idx] != '\n' && ptr[idx] != '\0'; idx++){
            std::cout<<ptr[idx];
          }
          std::cout<<std::endl;
        }


      }else if(res == 0){
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    }
  }).detach();
}




Json::Value HyprWorkspaces::executeQuery(const std::string& msg, std::string& err){
  int workSockfd;
  char buffer[8192];

  workSockfd = socket(AF_UNIX, SOCK_STREAM,0);
  sockaddr_un addr;

  addr.sun_family = AF_UNIX;

  // Establish Connection With Hyprland's UNIX Socket for Performing Workspace Related Action
  sockPath += ".socket.sock";
  strcpy(addr.sun_path, sockPath.c_str());
  sockPath = sockPath.substr(0, sockPath.rfind('/')+1);
  if(connect(workSockfd, (sockaddr*) &addr, sizeof(addr)) == -1){
    std::cerr<<"[IPC Connection Error] Unable to Establish Connection with hyprctl UNIX Socket"<<std::endl;
    return -3;
  }


  if(write(workSockfd, msg.c_str(), msg.size()) == -1){
    err = "Failed to Send Query";
    return -2;
  }
  if(read(workSockfd, buffer, sizeof(buffer)) <= 0){
    err = "Failed to Read Query Response";
    return -2;
  }


  // Closing the Connection
  if(close(workSockfd) == -1){
    std::cerr<<"[IPC Connection Error] Unable to Close Connection with hyprctl UNIX Socket"<<std::endl;
    return -3;
  }


  // Parsing the json response
  Json::Value root;
  std::istringstream jsonStream(buffer);
  if(!Json::parseFromStream(jsonReader, jsonStream, &root, &err)){
    return -1;
  }
  return root;
}

int HyprWorkspaces::GetActiveWorkspace(){
  std::string err;
  Json::Value workspaceJson = executeQuery("j/activeworkspace", err);
  if(workspaceJson == -1){
    std::cerr<<"[Error] Failed to parse Active Workspace Query Response ("<<err<<")"<<std::endl;
    return 1;
  }else if(workspaceJson == -2){
    std::cerr<<"[Error] In Active Workspace Query ("<<err<<")"<<std::endl;
    return 1;
  }

   activeWorkspaceId = workspaceJson["id"].asUInt();
   return 0;
}

Workspace HyprWorkspaces::GetActiveWorkspaceInfo(){
  auto ele = workspaces.find(activeWorkspaceId);
  if(ele == workspaces.end()){
    return Workspace {};
  }else{
    return ele->second;
  }
}

int HyprWorkspaces::GetWorkspaces(){
  std::string err;
  Json::Value workspacesJson = executeQuery("j/workspaces", err);
  if(workspacesJson == -1){
    std::cerr<<"[Error] Failed to parse Workspaces Query Response ("<<err<<")"<<std::endl;
    return 1;
  }else if(workspacesJson == -2){
    std::cerr<<"[Error] In Workspaces Query ("<<err<<")"<<std::endl;
    return 1;
  }

  for(Json::Value workspace : workspacesJson){
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