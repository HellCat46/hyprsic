#include "dbus/dbus.h"

class DbusSystem {
  public:
    DBusConnection *conn;
    DBusError err;

    DbusSystem(){
        dbus_error_init(&err);
        conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
        if(!conn){
            std::cerr<<"[Error] Failed to Connect With the DBUS Session: "<<err.name<<"\n[DBUS Error Message] : "<<err.message<<std::endl;
            return;
        }

        std::cout<<"[Info] Successfully created a connection to Dbus System Session."<<std::endl;
    }
    void getProperties(DBusMessageIter propIter, std::string *propNames, int propCount, DBusMessageIter *valueIters);

    ~DbusSystem(){
        dbus_error_free(&err);
        dbus_connection_flush(conn);
        dbus_connection_close(conn);
    }
};


void DbusSystem::getProperties(DBusMessageIter propIter, std::string *propNames, int propCount, DBusMessageIter *valueIters){
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