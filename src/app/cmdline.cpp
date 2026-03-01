#include "cmdline.hpp"
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

bool CLIClient::SendIPCCommand(std::string action, std::vector<std::string> args) {
    std::string command = action + " ";
    for (const auto& arg : args) {
        command += arg + " ";
    }
    
    int sockFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockFd < 0) {
        std::cout << "Failed to create socket" << std::endl;
        return true;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/hyprsic_ipc.sock", sizeof(addr.sun_path) - 1);
    
    if(connect(sockFd, (sockaddr *)&addr, sizeof(addr)) == 0){
        send(sockFd, command.c_str(), command.size(), 0);
        std::cout << "Sent command: " << command << std::endl;
        close(sockFd);
    }
    
    return false;
}