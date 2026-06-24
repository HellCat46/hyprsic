#include "gdkmm/display.h"
#include "giomm/application.h"
#include "glibmm/main.h"
#include "gtkmm/application.h"
#include "header/app.hpp"
#include "header/window.hpp"
#include "services/header/comm_bus.hpp"

#define TAG "Application UI"

Application::Application()
    : Gtk::Application("com.hellcat.hyprsic",
                       Gio::Application::Flags::DEFAULT_FLAGS),
      stat(&ctx.logger), mem(&ctx.logger), load(&ctx.logger), battery(&ctx),
      tempManager(&ctx), btManager(&ctx), btWindow(&ctx, &commBus, &btManager),
      notifManager(&ctx), notifWindow(&ctx, &commBus, &notifManager),
      mprisManager(&ctx), mprisWindow(&ctx, &commBus, &mprisManager),
      scrnsavrManager(&ctx), hyprInstance(&ctx.logger), snManager(&ctx),
      paManager(&ctx), paWindow(&ctx, &commBus, &paManager), wifiManager(&ctx),
      wifiWindow(&ctx, &commBus, &wifiManager), brtManager(&ctx),
      brtWindow(&ctx, &commBus, &brtManager),
      commBus(&ctx, &btManager, &mprisManager, &paManager, &scrnsavrManager,
              &wifiManager, &hyprInstance, &brtManager, &snManager) {}

Glib::RefPtr<Application> Application::create() {
  return Glib::RefPtr<Application>(new Application());
}

void Application::on_activate() {
  Gtk::Application::on_activate();
  // Required Because GTK doesn't care about layer shell windows for some reason...
  hold();
  
  auto dp = Gdk::Display::get_default();

  btManager.setup();
  hyprInstance.liveEventListener();
  captureSessionDBus();
  captureSystemDBus();

  cliIPCThread = std::thread(&Application::captureCLIIPC, this);

  ctx.initWindows();
  paWindow.init();
  btWindow.init();
  mprisWindow.init();
  notifWindow.init();
  brtWindow.init();
  wifiWindow.init();

  auto monitors = dp->get_monitors();
  int mCount = monitors->get_n_items();
  ctx.logger.LogInfo(TAG,
                     "on_activate: monitors count: " +
                     std::to_string(mCount));
  for (int idx = 0; idx < mCount; idx++) {
    auto monitor =
        std::dynamic_pointer_cast<Gdk::Monitor>(monitors->get_object(idx));

    mainWindows.push_back(std::unique_ptr<AppWindow>(new AppWindow(
        &ctx, &commBus, &hyprInstance, &snManager, &stat, &mem, &load,
        &battery, &tempManager, &scrnsavrManager, &mprisManager,
        &notifManager, &btManager, &brtManager, &paManager, &wifiManager)));

    mainWindows.back()->create(monitor, idx);
  }

  Glib::signal_timeout().connect(sigc::mem_fun(*this, &Application::UpdateUI),
                                 delay);

  dataUpdateThread = std::thread(&Application::UpdateData, this);
}

void Application::UpdateData() {
  while (true) {
    btManager.updateDevList();
    stat.UpdateData();
    tempManager.update();
    paManager.updateDevices();
    mprisManager.update();
    brtManager.update();
    wifiManager.update();

    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
  }
}

bool Application::UpdateUI() {
  btWindow.update();
  notifWindow.update();
  mprisWindow.update();
  brtWindow.update();
  paWindow.update();
  wifiWindow.update();

  for (auto &window : mainWindows) {
    window->update();
  }

  return true;
}

Application::~Application() {
  if (cliIPCThread.joinable())
    cliIPCThread.detach();
  if (dataUpdateThread.joinable())
    dataUpdateThread.detach();
}
