#include "iostream"
#include "sys/socket.h"
#include "sys/un.h"
#include "unistd.h"
#include "cstdlib"
class HyprWorkspaces {
  private:
  std::string sockPath;
  int evtSockfd;
  int workSockfd;


  public:
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
              std::cerr<<"Unable to Establish Connection with hyprctl UNIX Socket"<<std::endl;
              return 1;
      }

      // Establish Connection With Hyprland's UNIX Socket for Listening to Client events
      sockPath += ".socket2.sock";
      strcpy(addr.sun_path, sockPath.c_str());
      sockPath = sockPath.substr(0, sockPath.rfind('/')+1);
      if(connect(evtSockfd, (sockaddr*) &addr, sizeof(addr)) == -1){
        std::cerr<<"Unable to Establish Connection with Hyprland Socket UNIX Socket"<<std::endl;
        return 1;
      }

      char msg[] = "/dispatch workspace 6";
      char buffer[1024];
      std::cout<<write(workSockfd, msg, sizeof(msg))<<std::endl;
      read(workSockfd, buffer, sizeof(buffer));
      std::cout<<buffer<<std::endl;


      return 0;
    }

    int getPath(){
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

  ~HyprWorkspaces(){
      close(evtSockfd);
      close(workSockfd);
    }
};