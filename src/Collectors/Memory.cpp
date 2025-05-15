#include "iostream"
#include "fstream"
#include "cstring"

class Memory {
  private:
  std::ifstream memInfo;

  public:
    int Init(){
      memInfo.open("/proc/meminfo", std::ios::in);
      if(!memInfo.is_open()){
        std::cerr<<"[Init] Failed to Open Memory Info";
        return 1;
      }
      return 0;
    }

    long GetUsedRAM(){
      return parseInfo("MemTotal") - parseInfo("MemAvailable");
    }
    long GetTotRAM(){
      return parseInfo("MemTotal");
    }
    long GetUsedSwap(){
      return parseInfo("SwapTotal") - parseInfo("SwapFree");
    }
    long GetTotSwap(){
      return parseInfo("SwapTotal");
    }

    ~Memory(){
      memInfo.close();
    }

  private:
    long parseInfo(std::string type){
      long amt =0;
      char line[100];


      while(memInfo.getline(line, 100)){
        if(std::strncmp(line, type.c_str(), type.length()) == 0){
          int idx =0;
          while(line[idx] != '\0'){
            if(line[idx] >= 48 && line[idx] <= 57){
              amt = (amt*10) + (line[idx]-48);
            }
            idx++;
          }
          break;
        }
      }
      memInfo.clear();
      memInfo.seekg(0);

      //std::cout<<"\n"<<type<<" "<<amt<<std::endl;
      return amt;
    }
};