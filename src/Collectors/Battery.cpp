#include "iostream"
#include "fstream"

class Battery {
  std::ifstream capacity;
  std::ifstream status;
  public:
    int Init(){
      std::string basePath = "/sys/class/power_supply/BAT0/";

      capacity.open(basePath + "capacity", std::ios::in);
      status.open(basePath + "status", std::ios::in);

      if(!capacity.is_open() || !status.is_open()){
        std::cerr<<"[Init] Failed to Open Files Required for Battery Stats"<<std::endl;
        return 1;
      }
      return 0;
    }

    short getBatteryPercent(){
      short percent = 0;
      char ch;

      while(capacity.get(ch)){
        if(ch>=48 && ch<=57)
          percent = (percent*10) + (ch-48);
      }
      capacity.clear();
      capacity.seekg(0);

      return percent;
    }

    std::string getStatus(){
      char in[100];

      status.getline(in,100);
      status.clear();
      status.seekg(0);

      return in;
    }
};