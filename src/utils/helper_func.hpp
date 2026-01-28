#pragma once
#include <string>
#include <glib.h>

namespace HelperFunc {
bool saferStrCmp(const char *a, const char *b);
bool saferStrNCmp(const char *a, const char *b, int len);
gchar* ValidString(std::string str);
}