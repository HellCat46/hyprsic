#pragma once

#include <span>
#include <string>
#include <unordered_map>

class ResourceStore {
    public:
    std::unordered_map<std::string, std::span<const unsigned char>> icons;
    ResourceStore();  
};