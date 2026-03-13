#include "header/app.hpp"
#include <sys/socket.h>
#include <sys/un.h>

#define TAG "Application_CLI"

void Application::captureCLIIPC() {
  int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  sockaddr_un addr;

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  std::string socketPath = "/tmp/hyprsic_ipc.sock";
  strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

  unlink(addr.sun_path);
  int ret = bind(sockfd, (sockaddr *)&addr, sizeof(addr));
  if (ret == -1) {
    ctx.logger.LogError(TAG, "Failed to bind socket: " +
                                 std::string(strerror(errno)));
    return;
  }

  listen(sockfd, 1);

  while (true) {
    int clientFd = accept(sockfd, nullptr, nullptr);
    if (clientFd == -1) {
      ctx.logger.LogError(TAG, "Failed to accept connection: " +
                                   std::string(strerror(errno)));
      continue;
    }

    char buff[256];
    memset(buff, 0, sizeof(buff));

    ssize_t bytesRead = read(clientFd, buff, sizeof(buff) - 1);
    if (bytesRead <= 0) {
      ctx.logger.LogError(TAG, "Failed to read from socket: " +
                                   std::string(strerror(errno)));
      close(clientFd);
      continue;
    }

    std::string_view cmd(buff);

    ctx.logger.LogInfo(TAG, "Received IPC Command: " + std::string(cmd));

    size_t spacePos = cmd.find(' ');
    if (spacePos == std::string_view::npos) {
      ctx.logger.LogError(TAG, "Invalid command format received: " +
                                   std::string(cmd));
      close(clientFd);
      continue;
    }

    std::string_view action = cmd.substr(0, spacePos);
    cmd = cmd.substr(spacePos + 1);

    std::vector<std::string_view> args;
    while (true) {
      spacePos = cmd.find(' ');
      if (spacePos == std::string_view::npos) {
        break;
      }

      args.push_back(cmd.substr(0, spacePos));
      cmd = cmd.substr(spacePos + 1);
    }

    if (args.size() == 0) {
      ctx.logger.LogError(TAG, "No arguments provided for command: " +
                                   std::string(action));
      close(clientFd);
      continue;
    }

    handleActions(action, args);

    close(clientFd);
  }
}

void Application::handleActions(std::string_view action,
                                std::vector<std::string_view> args) {
  if (action == "toggle-view") {
    IPCToggleView(args[0]);
  } else if (action == "audio") {
    IPCCtrlAudioDev(args[0]);
  }
}

void Application::IPCToggleView(std::string_view module) {
  if (module == "pulseaudio") {

    ctx.showCtrlWindow("pulseaudio", 400, -1);
  } else if (module == "bluetooth") {

    ctx.showCtrlWindow("bluetooth", 400, 200);
  } else if (module == "notifications") {

    ctx.showCtrlWindow("notifications", 420, 400);
  } else if (module == "brightness") {

    ctx.showCtrlWindow("brightness", 340, 70);
  }
}

void Application::IPCCtrlAudioDev(std::string_view args) {
  if (args == "play-pause") {
    mprisManager.PlayPause();
  } else if (args == "toggle-mic") {
    paWindow.toggleMute(nullptr, nullptr, false);
  } else if (args == "toggle-output") {
    paWindow.toggleMute(nullptr, nullptr, true);
  }
}
