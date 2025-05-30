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
    bool discover;
    AppContext* ctx;
    std::unordered_map<std::string, Device> devices;
  	std::thread signalThread;

    void monitorChanges();
	int setDeviceProps(Device& dev, DBusMessageIter& propsIter);
  public:
    int getDeviceList();
    int connectDevice();
    int disconnectDevice();
    int removeDevice();
    int switchDiscovery();

    BluetoothManager(AppContext* context);
};
