#include "../debug/console_preview.hpp"
#include "chrono"
#include "cstdlib"
#include "cstring"
#include "iostream"
#include "thread"
#include "window.hpp"

int main(int argc, char **argv) {

  if (argc > 1 && std::strcmp(argv[1], "--debug-preview") == 0) {
    Display dp;
    while (true) {
      std::cout << dp.DisplayBar();
      std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return 0;
  }

  MainWindow win;
  win.RunApp();

  return 0;
}
