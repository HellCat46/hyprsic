#pragma once

#include <string>
#include <vector>
namespace CLIClient {
    bool SendIPCCommand(std::string action, std::vector<std::string> args);
}