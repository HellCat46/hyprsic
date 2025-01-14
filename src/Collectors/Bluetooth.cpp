#include "iostream"
#include "cstring"
#include "vector"
#include "dbus/dbus.h"

struct DeviceInfo {
  std::string iface, addr, name, icon;
  bool connected, mediaConnected;
  short batteryPer;
};

class BluetoothDevice {
    private:
      DBusConnection *conn;
      DBusError err;
      std::vector<DeviceInfo> devices;

      int getDeviceList();
      void getProperties(DBusMessageIter, std::string*, int,DBusMessageIter*);

    public:
      int Init(){
        dbus_error_init(&err);
        conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
        if(!conn){
          std::cerr<<"[Init] Failed to Connect With the DBUS Session: "<<err.name<<"\n[DBUS Error Message] : "<<err.message<<std::endl;
          return 1;
        }


//        if(dbus_bus_request_name(conn, "org.bluez", DBUS_NAME_FLAG_REPLACE_EXISTING, &err) == -1){
//          std::cerr<<"[Init] Failed to Create Request for Bluez on the DBUS Session: "<<err.name<<"\n[DBUS Error Message] : "<<err.message<<std::endl;
//          dbus_connection_close(conn);
//          return 1;
//        }

        getDeviceList();

//        for(auto device: devices){
//          std::cout<<"Device: "<<std::endl;
//
//          std::cout<<"\t"<<device.name<<std::endl;
//          std::cout<<"\t"<<device.addr<<std::endl;
//          std::cout<<"\t"<<device.iface<<std::endl;
//          std::cout<<"\t"<<device.icon<<std::endl;
//          std::cout<<"\t"<<device.connected<<std::endl;
//          std::cout<<"\t"<<device.mediaConnected<<std::endl;
//          std::cout<<"\t"<<device.batteryPer<<std::endl;
//        }
        return 0;
      }

      ~BluetoothDevice() {
        dbus_error_free(&err);
        dbus_connection_flush(conn);
        dbus_connection_close(conn);
      }
};

int BluetoothDevice::getDeviceList(){

        DBusMessage* msg = dbus_message_new_method_call("org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        if(!msg){
          std::cerr<<"Failed to create a message. "<<err.message<<std::endl;
          return 1;
        }

        DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
        if(!reply){
          std::cerr<<"Failed to get a reply. "<<err.message<<std::endl;
          return 1;
        }

        DBusMessageIter rootIter, entIter;
        dbus_message_iter_init(reply, &rootIter);

        if(dbus_message_iter_get_arg_type(&rootIter) != DBUS_TYPE_ARRAY){
          std::cerr<<"No Device Connected"<<std::endl;
          return -1;
        }

        dbus_message_iter_recurse(&rootIter, &entIter);


        while(dbus_message_iter_get_arg_type(&entIter) == DBUS_TYPE_DICT_ENTRY){
          DBusMessageIter entry, ifaceIter;
          char *objPath;


          dbus_message_iter_recurse(&entIter, &entry);
          dbus_message_iter_get_basic(&entry, &objPath);

          // Skip every entry (including endpoint and transport) except the actual device
          if(!(std::strlen(objPath) == 37 && std::strncmp(objPath, "/org/bluez/hci0/dev", 19) == 0)){
            dbus_message_iter_next(&entIter);
            continue;
          }

          dbus_message_iter_next(&entry);
          dbus_message_iter_recurse(&entry, &ifaceIter);

          DeviceInfo deviceInfo {
            "", "", "", "", false, false, -1
          };
          deviceInfo.iface = objPath;


          while(dbus_message_iter_get_arg_type(&ifaceIter) == DBUS_TYPE_DICT_ENTRY){
            DBusMessageIter ifaceEntry, propsIter;
            char* ifaceName;

            dbus_message_iter_recurse(&ifaceIter, &ifaceEntry);
            dbus_message_iter_get_basic(&ifaceEntry, &ifaceName);

            dbus_message_iter_next(&ifaceEntry);
            dbus_message_iter_recurse(&ifaceEntry, &propsIter);

            if(std::strcmp(ifaceName, "org.bluez.Device1") == 0){
              std::string properties[] = {"Address", "Connected", "Icon", "Name"};
              DBusMessageIter values[4];
              getProperties(propsIter, properties, 4, values);

              // For Address
              if(dbus_message_iter_get_arg_type(&values[0]) == DBUS_TYPE_STRING){
                char* value;
                dbus_message_iter_get_basic(&values[0], &value);
                deviceInfo.addr = value;
              }

              // For Connected
              if(dbus_message_iter_get_arg_type(&values[1]) == DBUS_TYPE_BOOLEAN){
                dbus_bool_t value;
                dbus_message_iter_get_basic(&values[1], &value);
                deviceInfo.connected = value;
              }

              // For Icon
              if(dbus_message_iter_get_arg_type(&values[2]) == DBUS_TYPE_STRING){
                char* value;
                dbus_message_iter_get_basic(&values[2], &value);
                deviceInfo.icon = value;
              }

              // For Device Name
              if(dbus_message_iter_get_arg_type(&values[3]) == DBUS_TYPE_STRING){
                char* value;
                dbus_message_iter_get_basic(&values[3], &value);
                deviceInfo.name = value;
              }

            }else if(std::strcmp(ifaceName, "org.bluez.Battery1") == 0){
              std::string properties[] = {"Percentage"};
              DBusMessageIter values[1];
              getProperties(propsIter, properties, 1, values);

              if(dbus_message_iter_get_arg_type(&values[0]) == DBUS_TYPE_BYTE){
                int value;
                dbus_message_iter_get_basic(&values[0], &value);
                deviceInfo.batteryPer = value;
              }
            }else if(std::strcmp(ifaceName, "org.bluez.MediaControl1") == 0){
              std::string properties[] = {"Connected"};
              DBusMessageIter values[1];
              getProperties(propsIter, properties, 1, values);

              // For Connected
              if(dbus_message_iter_get_arg_type(&values[1]) == DBUS_TYPE_BOOLEAN){
                dbus_bool_t value;
                dbus_message_iter_get_basic(&values[1], &value);
                deviceInfo.connected = value;
              }
            }



            dbus_message_iter_next(&ifaceIter);
          }

          devices.push_back(deviceInfo);
          dbus_message_iter_next(&entIter);
        }

        return 0;
}

void BluetoothDevice::getProperties(DBusMessageIter propIter, std::string *propNames, int propCount, DBusMessageIter *valueIters){
  while(dbus_message_iter_get_arg_type(&propIter) == DBUS_TYPE_DICT_ENTRY){
    DBusMessageIter entry, valueVariant;
    char* propertyName;


    dbus_message_iter_recurse(&propIter, &entry);
    dbus_message_iter_get_basic(&entry, &propertyName);

    dbus_message_iter_next(&entry);
    dbus_message_iter_recurse(&entry, &valueVariant);

    int idx =0;
    while(idx < propCount){
  		//std::cout<<propertyName<<(dbus_message_iter_get_arg_type(&valueVariant))<<std::endl;

      	if(std::strcmp(propertyName, propNames[idx].c_str()) == 0){
        	valueIters[idx] = valueVariant;
      	}
        idx++;
    }

    dbus_message_iter_next(&propIter);
  }
}