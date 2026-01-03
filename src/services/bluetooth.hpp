#pragma once

#include "cstring"
#include "thread"
#include "string"
#include "../app/context.hpp"
#include "unordered_map"


enum DeviceType {
  Speaker,
  Microphone,
  Headphone,
  Phone,
  Unknown
};


struct Device {
  std::string address, adapter, name;
  short rssi;
  DeviceType device;
  bool paired, trusted;
};


class BluetoothManager {
  private:
    AppContext* ctx;
  	std::thread signalThread;

    void monitorChanges();
	int setDeviceProps(Device& dev, DBusMessageIter& propsIter);
  public:
    bool discovering, power;
    std::unordered_map<std::string, Device> devices;
    
    int getDeviceList();
    int connectDevice();
    int disconnectDevice();
    int removeDevice();
    
    int switchDiscovery(bool on);
    int switchPower(bool on);

    BluetoothManager(AppContext* context);
};
