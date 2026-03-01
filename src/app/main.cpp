#include "../debug/console_preview.hpp"
#include "CLI/CLI.hpp"
#include "chrono"
#include "cmdline.hpp"
#include "cstdlib"
#include "cstring"
#include "iostream"
#include "thread"
#include "window.hpp"

int main(int argc, char **argv) {

  CLI::App cliApp{"HyprSic - A Status Bar and Control Center for Hyprland"};

  bool debugPreview = false;
  cliApp.add_flag(
      "--debug-preview", debugPreview,
      "Launch a Debug Preview of the Bar Infomation in the Terminal");

  std::string winName;
  auto toggleCmd =
      cliApp.add_subcommand("toggle-view", "Toggle a specific Window (View & Hide)");
  toggleCmd->add_option("module", winName, "Module to Toggle")
      ->required()
      ->check(CLI::IsMember(
          {"pulseaudio", "bluetooth", "notifications", "media", "brightness"}));

  CLI11_PARSE(cliApp, argc, argv);

  if (debugPreview) {
    Display dp;

    while (true) {
      std::cout << dp.DisplayBar();
      std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return 0;
  }

  if (!winName.empty()) {
    return CLIClient::SendIPCCommand("toggle-view", {winName});
  }

  Application app;
  app.Run(argc, argv);

  return 0;
}
