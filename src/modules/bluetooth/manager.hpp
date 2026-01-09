#pragma once

#include "cstring"
#include "dbus/dbus.h"
#include "thread"
#include "string"
#include "../../app/context.hpp"
#include "unordered_map"
#include <string>
#include <unordered_map>


struct Device {
  std::string addr, name, path, deviceType;
  short rssi;
  bool paired, trusted, connected, mediaConnected;
  short batteryPer;
};




class BluetoothManager {
  private:
    AppContext* ctx;
    DBusMessage* devListMsg;
    LoggingManager* logger;
  	std::thread signalThread;

    void monitorChanges();
	void setDeviceProps(Device& dev, DBusMessageIter& propsIter);
	int getPropertyVal(const char* prop);
  public:
    bool discovering, power;
    std::unordered_map<std::string, Device> devices;
    
    int getDeviceList();
    bool printDevicesInfo();
    
    // Device Operations
    int connectDevice(bool state, const char* devPath);
    int removeDevice(bool state, const char* devPath);
    int trustDevice(bool state, const char* devPath);
    
    int switchDiscovery(bool on);
    int switchPower(bool on);

    BluetoothManager(AppContext* context, LoggingManager* logMgr);
};


