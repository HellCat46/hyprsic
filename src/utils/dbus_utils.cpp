#include "dbus_utils.hpp"
#include "cstring"
#include "iostream"

void DbusUtils::getProperties(DBusMessageIter propIter, std::string *propNames,
                              int propCount, DBusMessageIter *valueIters) {

  while (dbus_message_iter_get_arg_type(&propIter) == DBUS_TYPE_DICT_ENTRY) {
    DBusMessageIter entry, valueVariant;
    char *propertyName;

    dbus_message_iter_recurse(&propIter, &entry);
    dbus_message_iter_get_basic(&entry, &propertyName);

    dbus_message_iter_next(&entry);
    dbus_message_iter_recurse(&entry, &valueVariant);

    // std::cout << propertyName << (dbus_message_iter_get_arg_type(&valueVariant))
    //           << std::endl;

    for(int idx =0; idx < propCount; idx++) {
      if(propNames[idx][0] == ' ') continue;  
        
      // std::cout << "\t\t" << propertyName << " "
      //           << (dbus_message_iter_get_arg_type(&valueVariant)) << " "
      //           << propNames[idx] << std::endl;

      if (std::strcmp(propertyName, propNames[idx].c_str()) == 0) {
        valueIters[idx] = valueVariant;
        propNames[idx][0] = ' ';
        break;
      }
    }

    dbus_message_iter_next(&propIter);
  }
}
