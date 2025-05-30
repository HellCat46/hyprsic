#include "dbus_utils.hpp"
#include "cstring"

void DbusUtils::getProperties(DBusMessageIter propIter, std::string *propNames,
                   int propCount, DBusMessageIter *valueIters) {
  while (dbus_message_iter_get_arg_type(&propIter) == DBUS_TYPE_DICT_ENTRY) {
    DBusMessageIter entry, valueVariant;
    char *propertyName;

    dbus_message_iter_recurse(&propIter, &entry);
    dbus_message_iter_get_basic(&entry, &propertyName);

    dbus_message_iter_next(&entry);
    dbus_message_iter_recurse(&entry, &valueVariant);

    int idx = 0;
    while (idx < propCount) {
      // std::cout<<propertyName<<(dbus_message_iter_get_arg_type(&valueVariant))<<std::endl;

      if (std::strcmp(propertyName, propNames[idx].c_str()) == 0) {
        valueIters[idx] = valueVariant;
        propNames[idx][0] = ' ';
      }
      idx++;
    }

    dbus_message_iter_next(&propIter);
  }
}