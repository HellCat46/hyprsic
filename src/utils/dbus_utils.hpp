#pragma once
#include "dbus/dbus.h"
#include "string"

namespace DbusUtils {
void getProperties(DBusMessageIter propIter, std::string *propNames,
                   int propCount, DBusMessageIter *valueIters);
} // namespace DbusUtils
