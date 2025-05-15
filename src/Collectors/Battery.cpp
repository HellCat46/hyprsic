#include "iostream"
#include "fstream"
#include "dirent.h"
#include "cstring"
#include "vector"

class Battery {
  std::ifstream capacity;
  std::ifstream status;
  std::string name;
  public:
  	bool failed;
    int Init(std::string basePath){
      std::cout<<"[Init Info] Initiating Info Object for Battery "<<basePath.substr(basePath.rfind("/")+1, basePath.size())<<"."<<std::endl;
      capacity.open(basePath + "/capacity", std::ios::in);
      status.open(basePath + "/status", std::ios::in);

      if(!capacity.is_open() || !status.is_open()){
        std::cerr<<"[Init Error] Failed to Open Files Required for Battery Stats"<<std::endl;

        failed = 1;
        return 1;
      }

      std::ifstream manufacturer, model;
      manufacturer.open(basePath + "/manufacturer", std::ios::in);
      model.open(basePath + "/model_name", std::ios::in);

      if(!manufacturer.is_open() || !model.is_open()){
        std::cerr<<"[Init Error] Failed to Get Battery Manufacturer or Model Name"<<std::endl;
        name = "Unknown";
      }else{
        char in[30];
        model.getline(in,30);
        name += in;
        manufacturer.getline(in, 30);
        name += " (";
        name += in;
        name += ')';

        //std::cout<<name<<std::endl;

        model.close();
        manufacturer.close();
      }

      failed = 0;
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

    ~Battery(){
      capacity.close();
      status.close();
    }
};

class BatteryInfo {
  Battery* batteries;
  int battCount;
  public:
    int Init(){
      DIR *dir;
      std::string basePath = "/sys/class/power_supply/";
      char folderStr[] = "BAT";

      dir = opendir(basePath.c_str());

      if(dir == nullptr){
        return 1;
      }

	  std::vector<std::string> battPaths;
	  for(dirent* ent = readdir(dir); ent != nullptr; ent = readdir(dir)){
            if(std::strncmp(folderStr, ent->d_name, 3) == 0){
              battPaths.push_back(ent->d_name);
            }
	  }

      battCount = battPaths.size();
      batteries = new Battery[battCount];

      int idx =0;
      for(auto batt : battPaths){
        batteries[idx++].Init(basePath + batt);
      }

      return 0;
    }

    short getTotPercent(){
      float avg = 0;
      char ch;

      for(int idx =0; idx < battCount; idx++){
        if(batteries[idx].failed) continue;
        avg = ((avg*(idx)) + batteries[idx].getBatteryPercent())/(idx+1);
      }

      return avg;
    }

    int getBatteryCount(){
      return battCount;
    }

    ~BatteryInfo(){
      delete[] batteries;
    }
};