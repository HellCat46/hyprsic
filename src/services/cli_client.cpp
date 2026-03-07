#include "header/cli_client.hpp"
#include "../debug/console_preview.hpp"
#include "CLI/CLI.hpp"
#include "chrono"
#include "iostream"
#include "thread"
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

void CLIClient::Run(int argc, char **argv) {
  CLI::App cliApp{"HyprSic - A Status Bar and Control Center for Hyprland"};

  bool debugPreview = false;
  cliApp.add_flag(
      "--debug-preview", debugPreview,
      "Launch a Debug Preview of the Bar Infomation in the Terminal");

  // View Toggle Command
  std::string winName;
  auto viewCmd = cliApp.add_subcommand(
      "toggle-view", "Toggle a specific Window (View & Hide)");
  viewCmd->add_option("module", winName, "Module to Toggle")
      ->required()
      ->check(CLI::IsMember(
          {"pulseaudio", "bluetooth", "notifications", "media", "brightness"}));
  
  // Audio Control Command
  std::string audioAction;
  auto audioCmd = cliApp.add_subcommand(
      "audio", "Control Audio Devices (play/pause, mute/unmute mic, and mute/unmute output)");
  audioCmd->add_option("action", audioAction, "Audio Action to Perform")
      ->required()
      ->check(CLI::IsMember({"play-pause", "toggle-mic", "toggle-output"}));
    
  try {
    cliApp.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    std::exit(cliApp.exit(e));
  }

  if (debugPreview) {
    Display dp;

    while (true) {
      std::cout << dp.DisplayBar();
      std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    exit(0);
  }

  if (!winName.empty()) {
    exit(CLIClient::SendIPCCommand("toggle-view", {winName}));
  }
  
  if (!audioAction.empty()) {
    exit(CLIClient::SendIPCCommand("audio", {audioAction}));
  }
}

bool CLIClient::SendIPCCommand(std::string action,
                               std::vector<std::string> args) {
  std::string command = action + " ";
  for (const auto &arg : args) {
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

  if (connect(sockFd, (sockaddr *)&addr, sizeof(addr)) == 0) {
    send(sockFd, command.c_str(), command.size(), 0);
    std::cout << "Sent command: " << command << std::endl;
    close(sockFd);
  }

  return false;
}
