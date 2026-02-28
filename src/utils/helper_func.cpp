#include "helper_func.hpp"
#include "glib.h"
#include <cstring>

bool HelperFunc::saferStrCmp(const char *a, const char *b) {
  if (a == nullptr || b == nullptr)
    return false;

  if (std::strlen(a) != std::strlen(b))
    return false;

  return (std::strcmp(a, b) == 0);
}

bool HelperFunc::saferStrNCmp(const char *a, const char *b, int len) {
  if (a == nullptr || b == nullptr)
    return false;

  if (std::strlen(a) < static_cast<size_t>(len) ||
      std::strlen(b) < static_cast<size_t>(len))
    return false;

  return (std::strncmp(a, b, len) == 0);
}

gchar *HelperFunc::ValidString(std::string str) {
  if (str.empty())
    return g_strdup("");

  gchar *utf8Text = g_utf8_make_valid(str.c_str(), -1);
  gchar *validStr = g_markup_escape_text(utf8Text, -1);
  g_free(utf8Text);

  return validStr;
}

std::string HelperFunc::convertToTime(int minutes) {
  if (minutes < 0)
    return "Unknown";

  int hours = minutes / 60;
  int mins = minutes % 60;

  std::string timeStr;
  if (hours > 0) {
    timeStr += std::to_string(hours) + "h ";
  }
  if (mins > 0) {
    timeStr += std::to_string(mins) + "m";
  }

  return timeStr;
}
