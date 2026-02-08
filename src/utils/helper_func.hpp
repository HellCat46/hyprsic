#pragma once
#include <glib.h>
#include <string>

namespace HelperFunc {
bool saferStrCmp(const char *a, const char *b);
bool saferStrNCmp(const char *a, const char *b, int len);
std::string convertToTime(int minutes);
gchar *ValidString(std::string str);
}
