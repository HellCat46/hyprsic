#include "iostream"
#include "sys/socket.h"
#include "sys/un.h"
#include "json/json.h"
#include "vector"
#include "unistd.h"
#include "cstdlib"
#include "sstream"


struct Workspace {
  unsigned int id, monitorId;
  std::string name, monitor;
  bool fullScreen;
};

class HyprWorkspaces {
  private:
  std::string sockPath;
  int evtSockfd;
  int workSockfd;
  Workspace* activeWorkspace;
  std::vector<Workspace> workspaces;
  Json::CharReaderBuilder jsonReader;

  int getPath();
  Json::Value executeQuery(const std::string&, std::string&);

  public:

    int GetWorkspaces();
    int Init(){
      if(getPath()){
        return 1;
      }

      sockaddr_un addr;
      addr.sun_family = AF_UNIX;

      evtSockfd = socket(AF_UNIX, SOCK_STREAM,0);
      workSockfd = socket(AF_UNIX, SOCK_STREAM,0);



      // Establish Connection With Hyprland's UNIX Socket for Performing Workspace Related Action
      sockPath += ".socket.sock";
      strcpy(addr.sun_path, sockPath.c_str());
      sockPath = sockPath.substr(0, sockPath.rfind('/')+1);
      if(connect(workSockfd, (sockaddr*) &addr, sizeof(addr)) == -1){
              std::cerr<<"[Init Error] Unable to Establish Connection with hyprctl UNIX Socket"<<std::endl;
              return 1;
      }

      // Establish Connection With Hyprland's UNIX Socket for Listening to Client events
      sockPath += ".socket2.sock";
      strcpy(addr.sun_path, sockPath.c_str());
      sockPath = sockPath.substr(0, sockPath.rfind('/')+1);
      if(connect(evtSockfd, (sockaddr*) &addr, sizeof(addr)) == -1){
        std::cerr<<"[Init Error] Unable to Establish Connection with Hyprland Socket UNIX Socket"<<std::endl;
        return 1;
      }

      return 0;
    }

  ~HyprWorkspaces(){
      close(evtSockfd);
      close(workSockfd);
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

Json::Value HyprWorkspaces::executeQuery(const std::string& msg, std::string& err){
  char buffer[8192];
  if(write(workSockfd, msg.c_str(), msg.size()) == -1){
    err = "Error While Sending Query";
    return -1;
  }


  if(read(workSockfd, buffer, sizeof(buffer)) <= 0){
    err = "Error While Reading Response";
    return -1;
  }

  Json::Value root;


  std::istringstream jsonStream(buffer);


  if(!Json::parseFromStream(jsonReader, jsonStream, &root, &err)){
    return -1;
  }

  return root;
}



int HyprWorkspaces::GetWorkspaces(){
  std::string err;
  Json::Value workspacesJson = executeQuery("j/workspaces", err);

  if(workspacesJson == -1){
    std::cerr<<"[Error] Failed to parse Workspaces Query Response ("<<err<<")"<<std::endl;
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
    workspaces.push_back(wSpace);
  }

  for(auto workspace : workspaces){
    std::cout<<workspace.id<<std::endl;
    std::cout<<workspace.monitorId<<std::endl;
    std::cout<<workspace.name<<std::endl;
    std::cout<<workspace.monitor<<std::endl;
    std::cout<<workspace.fullScreen<<std::endl;
    std::cout<<std::endl;
  }

  return 0;
}