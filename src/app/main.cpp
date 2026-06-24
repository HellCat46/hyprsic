#include "header/app.hpp"
#include "services/header/cli_client.hpp"

int main(int argc, char **argv) {
  CLIClient::Run(argc, argv);

  // g_log_set_always_fatal((GLogLevelFlags)(G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));

  auto app = Application::create();
  return app->run(argc, argv);
}
