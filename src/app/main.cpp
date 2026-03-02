#include "cmdline.hpp"
#include "cstdlib"
#include "cstring"
#include "window.hpp"

int main(int argc, char **argv) {
  CLIClient::Run(argc, argv);

  Application app;

  return app.Run(argc, argv);
}
