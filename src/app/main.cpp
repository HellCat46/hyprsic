#include "header/app.hpp"
#include "cstdlib"
#include "cstring"
#include "services/header/cli_client.hpp"

int main(int argc, char **argv) {
  CLIClient::Run(argc, argv);

  Application app;

  return app.Run(argc, argv);
}
