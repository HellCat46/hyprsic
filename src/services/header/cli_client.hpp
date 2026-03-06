#pragma once

#include <string>
#include <vector>
namespace CLIClient {
    void Run(int argc, char** argv); 
    bool SendIPCCommand(std::string action, std::vector<std::string> args);
}