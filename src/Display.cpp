#include "iostream"
#include "math.h"
#include "chrono"
#include "format"
#include "Hyprland/Workspaces.cpp"
#include "Collectors/Network.cpp"
#include "Collectors/SysLoad.cpp"
#include "Collectors/Memory.cpp"
#include "Collectors/Disk.cpp"
#include "Collectors/Battery.cpp"

struct Stats {
  double rx, tx;
  unsigned long diskAvail, diskTotal;
  std::chrono::time_point<std::chrono::steady_clock> time;
};

class Display {
  Stats stat;
  Network net;
  SysLoad load;
  Memory mem;
  Disk disk;
  BatteryInfo battery;
  HyprWorkspaces hyprWS;
  public:
   Display() {
     // Initiating The Network and It's Stats
     net.Init();
     stat.rx = net.GetTotRx();
     stat.tx = net.GetTotTx();
     stat.time = std::chrono::steady_clock::now();

     load.Init();
     mem.Init();

     disk.Init("/home/hellcat");

     battery.Init();

     hyprWS.Init();
   }
  std::string DisplayBar(){
    std::string str;

    auto curTime = std::chrono::steady_clock::now();
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(curTime - stat.time).count();


    double trx = net.GetTotRx(), ttx = net.GetTotTx();
    str += DisplayBytes(trx,2) + " (" + DisplayBytes((trx-stat.rx)/sec,1) + ")\t"+ DisplayBytes(ttx,2) + " ("+DisplayBytes((ttx-stat.tx)/sec,1)+ ")\t";
    str += std::to_string(load.GetLoad(1)) +" "+std::to_string(load.GetLoad(5)) +" "+std::to_string(load.GetLoad(15)) +"\t";
    str += DisplayBytes(mem.GetUsedRAM()*1024,2) + "/" + DisplayBytes(mem.GetTotRAM()*1024,2) + " " + DisplayBytes(mem.GetUsedSwap()*1024,2) + "/" + DisplayBytes(mem.GetTotSwap()*1024,2) + "\t";

    if(disk.GetDiskInfo(&stat.diskAvail, &stat.diskTotal) == 0){
      //std::cout<<stat.diskAvail<<std::endl;
      str += DisplayBytes(stat.diskTotal-stat.diskAvail, 2) + "/" + DisplayBytes(stat.diskTotal, 2) + "\t";
    }

    str += std::to_string(battery.getTotPercent()) + "\t"; //+ "% (" + battery.getStatus() + ")\t";


    if(hyprWS.GetActiveWorkspace() == 0){
      Workspace ws = hyprWS.GetActiveWorkspaceInfo();

      str += "Active WS: "+ ws.name + "\t";
    }



    str += "\n";
    stat.rx = trx;
    stat.tx = ttx;
    stat.time = curTime;
    return str;
  }
  std::string DisplayBytes(double bytes, int precision){
    std::string str;
    precision++;

    //std::cout<<std::fixed<<bytes<<"\t"<<pow(1024,3)<<std::endl;
    if(bytes>pow(1024,3)){ // GigaBytes
      str = std::to_string(bytes/pow(1024,3));
      str = str.substr(0, str.find('.')+precision);
      str += " GiB";
    }else if(bytes>pow(1024,2)){ // MegaBytes
      str = std::to_string(bytes/pow(1024,2));
      str = str.substr(0, str.find('.')+precision);
      str +=  " MiB";
    }else if(bytes>1024){ // KiloBytes
      str = std::to_string(bytes/1024);
      str = str.substr(0, str.find('.')+precision);
      str +=  " KiB";
    }else if(bytes>0) { // Bytes
      str = std::to_string(bytes);
      str = str.substr(0, str.find('.'));
      str +=  " B";
    }else {
      str += "0 B";
    }
    return str;
  }
};
