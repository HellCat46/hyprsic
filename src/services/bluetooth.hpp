#pragma once

#include "cstring"
#include "dbus/dbus.h"
#include "gtk/gtk.h"
#include "thread"
#include "string"
#include "../app/context.hpp"
#include "unordered_map"
#include <string>
#include <unordered_map>


struct Device {
  std::string addr, name, path, deviceType;
  short rssi;
  bool paired, trusted, connected, mediaConnected;
  short batteryPer;
};


struct FuncArgs {
  AppContext* ctx;
  char* devIfacePath;
  bool state;
};

class BluetoothManager {
  private:
    AppContext* ctx;
    DBusMessage* devListMsg;
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
    static int connectDevice(GtkWidget *widget, gpointer user_data);
    static int removeDevice(GtkWidget *widget, gpointer user_data);
    static int trustDevice(FuncArgs args);
    
    int switchDiscovery(bool on);
    int switchPower(bool on);

    BluetoothManager(AppContext* context);
};


