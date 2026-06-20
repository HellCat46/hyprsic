#include "header/app.hpp"
#include "services/header/cli_client.hpp"

int main(int argc, char **argv) {
  CLIClient::Run(argc, argv);

  auto app = Application::create();
  return app->run(argc, argv);
}
