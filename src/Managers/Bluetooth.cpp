#include "iostream"
#include "cstring"
#include "vector"
#include "thread"
#include "../BaseInstances/DbusSystem.cpp"
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
    DbusSystem* dbus;
    std::unordered_map<std::string, Device> devices;
  	std::thread signalThread;

    void monitorChanges();
  public:
    int getDeviceList();
    int connectDevice();
    int disconnectDevice();
    int removeDevice();
    int switchDiscovery();

    BluetoothManager(DbusSystem* dbusInstance){
      dbus = dbusInstance;
      discover = false;
      signalThread = std::thread(&BluetoothManager::monitorChanges, this);
    }
};

int BluetoothManager::switchDiscovery(){
  if(discover){

    std::cout<<"[INFO] Turning off Bluetooth Discovery."<<std::endl;
    discover = false;
  }else {
    DBusMessage* msg = dbus_message_new_method_call("org.bluez", "/org/bluez/hci0", "org.bluez.Adapter1", "StartDiscovery");
    if(!msg){
      std::cerr<<"Failed to create a message. "<<dbus->err.message<<std::endl;
      return 1;
    }

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(dbus->conn, msg, -1, &(dbus->err));
    if(!reply){
      std::cerr<<"Failed to get a reply. "<<dbus->err.message<<std::endl;
      return 1;
    }

    std::cout<<"[INFO] Turning on Bluetooth Discovery."<<std::endl;
    this->discover = true;
  }
  return 0;
}

void BluetoothManager::monitorChanges() {
  std::cout<<"[Info] Adding filter to Bluez Signals"<<std::endl;


  // Adding Filters to Signals before starting listening to them
  dbus_bus_add_match(dbus->conn, "type='signal', interface='org.freedesktop.DBus.ObjectManager', member='InterfacesAdded'", &(dbus->err));
  if(dbus_error_is_set(&(dbus->err))){
    std::cerr<<"Failed to add filter for Signal Member InterfaceAdded"<<std::endl;
    return;
  }

  dbus_bus_add_match(dbus->conn, "type='signal', interface='org.freedesktop.DBus.ObjectManager', member='InterfacesRemoved'", &(dbus->err));
  if(dbus_error_is_set(&(dbus->err))){
    std::cerr<<"Failed to add filter for Signal Member InterfaceRemoved"<<std::endl;
    return;
  }

  dbus_bus_add_match(dbus->conn, "type='signal', interface='org.freedesktop.DBus.Properties', member='PropertiesChanged'", &(dbus->err));
  if(dbus_error_is_set(&(dbus->err))){
    std::cerr<<"Failed to add filter for Signal Member PropestiesChanged"<<std::endl;
    return;
  }
  std::cout<<"[Info] Successfully Added Filters to Bluez Dbus Signals. Started listening to events now."<<std::endl;

  DBusMessage* msg;
  while(true){
    // Blocks the thread until new message received
    if(!dbus_connection_read_write(dbus->conn, 0)){
      std::cerr<<"Connection Closed while Waiting for Signal Messages"<<std::endl;
      return;
    }

    msg = dbus_connection_pop_message(dbus->conn);
    if(!msg){
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }
    std::cout<<"[Info] Received Signal Message"<<std::endl;

    DBusMessageIter rootIter, entIter, devIter, propsIter;
    dbus_message_iter_init(msg, &rootIter);


    if(dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager", "InterfacesAdded")){
      std::cout<<"[Info] Received InterfacesAdded Signal"<<std::endl;


      if(dbus_message_iter_get_arg_type(&rootIter) != DBUS_TYPE_OBJECT_PATH){
        std::cerr<<"Unable to parse InterfacesAdded Reply. Unknown Format (The Root is not an array.)"<<std::endl;
        continue;
      }

      char* value;
      dbus_message_iter_get_basic(&rootIter, &value);
      std::cout<<value<<std::endl;


      // Moving to next entry after objectPath entry
      dbus_message_iter_next(&rootIter);
      dbus_message_iter_recurse(&rootIter, &entIter);
      if(dbus_message_iter_get_arg_type(&entIter) != DBUS_TYPE_DICT_ENTRY){
        std::cerr<<"Unable to parse InterfacesAdded Reply. Unknown Format (The Second Entry is not an Object.)"<<std::endl;
        continue;
      }


      // Looking for Device1 Entry
      while(dbus_message_iter_get_arg_type(&entIter) == DBUS_TYPE_DICT_ENTRY){
        char* objPath;

        dbus_message_iter_recurse(&entIter, &devIter);
      	dbus_message_iter_get_basic(&devIter, &objPath);
      	if(std::strncmp(objPath, "org.bluez.Device1", 17) == 0){
            break;
      	}
        dbus_message_iter_next(&entIter);
      }

      if(dbus_message_iter_get_arg_type(&entIter) != DBUS_TYPE_DICT_ENTRY){
        std::cout<<"Unable to find Device1 Entry in InterfacesAdded Reply."<<std::endl;
        continue;
      }


	    // Getting Properties
      dbus_message_iter_next(&devIter);
      dbus_message_iter_recurse(&devIter, &propsIter);
      std::string properties[] = {"Adapter", "Address", "Connected", "Paired", "RSSI", "Trusted"};
      DBusMessageIter values[6];
      dbus->getProperties(propsIter, properties, 6, values);

      // Check whether Device is already connected
      if(dbus_message_iter_get_arg_type(&values[2]) == DBUS_TYPE_BOOLEAN){
        dbus_bool_t value;
        dbus_message_iter_get_basic(&values[2], &value);
        if(value) continue;
      }

      Device dev {"", "", "", -110, DeviceType::Unknown, false, false};
      // For Adapter Path
      if(dbus_message_iter_get_arg_type(&values[0]) == DBUS_TYPE_OBJECT_PATH){
        char* value;
        dbus_message_iter_get_basic(&values[0], &value);
        dev.adapter = value;
      }

      // For Address
      if(dbus_message_iter_get_arg_type(&values[1]) == DBUS_TYPE_STRING){
        char* value;
        dbus_message_iter_get_basic(&values[1], &value);
        dev.address = value;
      }


      // For Paired
      if(dbus_message_iter_get_arg_type(&values[3]) == DBUS_TYPE_BOOLEAN){
        dbus_bool_t value;
        dbus_message_iter_get_basic(&values[3], &value);
        dev.paired = value;
      }

      // For RSSI
      if(dbus_message_iter_get_arg_type(&values[4]) == DBUS_TYPE_INT16){
        dbus_int16_t value;
        dbus_message_iter_get_basic(&values[4], &value);
        dev.rssi = value;
      }

      // For Trusted
      if(dbus_message_iter_get_arg_type(&values[5]) == DBUS_TYPE_BOOLEAN){
        dbus_bool_t value;
        dbus_message_iter_get_basic(&values[5], &value);
        dev.trusted = value;
      }

      devices.insert({dev.address, dev});
      std::cout<<"[Info] Added Device to Device List"<<std::endl;
    }else if(dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager", "InterfacesRemoved")){
      std::cout<<"[Info] Received InterfacesRemoved Signal"<<std::endl;
    }else if(dbus_message_is_signal(msg, "org.freedesktop.DBus.Properties", "PropertiesChanged")){
      std::cout<<"[Info] Received PropertiesChanged Signal"<<std::endl;
    }else {
      std::cerr<<"[Info] Received Unknown Signal"<<std::endl;
    }

    dbus_message_unref(msg);
  }
  return;
}

int BluetoothManager::getDeviceList(){
  if(devices.size() == 0) return 1;

  for(auto& [_, value]: devices){
      std::cout<<"Device: "<<std::endl;
      std::cout<<"\t"<<value.name<<std::endl;
      std::cout<<"\t"<<value.address<<std::endl;
      std::cout<<"\t"<<value.adapter<<std::endl;
      std::cout<<"\t"<<value.paired<<std::endl;
      std::cout<<"\t"<<value.trusted<<std::endl;
      std::cout<<"\t"<<value.rssi<<std::endl;
  }

  std::cout<<"[Info] Device Count: "<<devices.size()<<std::endl;
  return 0;
}