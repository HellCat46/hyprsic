#include "helper_func.hpp"
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
  
  if (std::strlen(a) < static_cast<size_t>(len) || std::strlen(b) < static_cast<size_t>(len))
    return false;

  return (std::strncmp(a, b, len) == 0);
}