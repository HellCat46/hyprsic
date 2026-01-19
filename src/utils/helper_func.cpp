#include "helper_func.hpp"
#include <cstring>

bool HelperFunc::saferStrCmp(const char *a, const char *b) {
  if (a == nullptr || b == nullptr)
    return false;
  
  if (std::strlen(a) != std::strlen(b))
    return false;

  return (std::strcmp(a, b) == 0);
}
