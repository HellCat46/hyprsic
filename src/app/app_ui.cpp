#include "header/app.hpp"
#include "header/window.hpp"
#include "services/header/comm_bus.hpp"

Application::Application()
    : stat(&ctx.logger), mem(&ctx.logger), load(&ctx.logger), battery(&ctx),
      tempManager(&ctx), btManager(&ctx), btWindow(&ctx, &commBus, &btManager),
      notifManager(&ctx), notifWindow(&ctx, &commBus, &notifManager),
      mprisManager(&ctx), mprisWindow(&ctx, &commBus, &mprisManager),
      scrnsavrManager(&ctx), hyprInstance(&ctx.logger), snManager(&ctx),
      paManager(&ctx), paWindow(&ctx, &commBus, &paManager), wifiManager(&ctx),
      wifiWindow(&ctx, &commBus, &wifiManager), brtManager(&ctx),
      brtWindow(&ctx, &commBus, &brtManager),
      commBus(&ctx, &btManager, &mprisManager, &paManager, &scrnsavrManager,
              &wifiManager, &hyprInstance, &brtManager, &snManager) {

  app = gtk_application_new("com.hellcat.hyprsic", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), this);
}

int Application::Run(int argc, char **argv) {
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}

void Application::activate(GtkApplication *app, gpointer user_data) {
  Application *self = static_cast<Application *>(user_data);
  GdkDisplay *display = gdk_display_get_default();

  self->btManager.setup();
  self->hyprInstance.liveEventListener();
  self->ssnDBusThread = std::thread(&Application::captureSessionDBus, self);
  self->sysDBusThread = std::thread(&Application::captureSystemDBus, self);
  self->cliIPCThread = std::thread(&Application::captureCLIIPC, self);

  self->ctx.initWindows();
  self->paWindow.setupIcons();
  self->paWindow.init();
  self->btWindow.init();
  self->mprisWindow.init();
  self->notifWindow.init();
  self->brtWindow.init();
  self->wifiWindow.init();
  // clipboardManager.init(display);

  int mCount = gdk_display_get_n_monitors(display);
  for (int idx = 0; idx < mCount; idx++) {
    self->mainWindows.push_back(std::unique_ptr<Window>(new Window(
        &self->ctx, &self->commBus, &self->hyprInstance, &self->snManager, &self->stat,
        &self->mem, &self->load, &self->battery, &self->tempManager,
        &self->scrnsavrManager, &self->mprisManager, &self->mprisWindow,
        &self->notifManager, &self->notifWindow, &self->btManager,
        &self->btWindow, &self->brtManager, &self->brtWindow, &self->paManager,
        &self->paWindow, &self->wifiManager, &self->wifiWindow)));

    self->mainWindows.back()->create(app, display, idx);
  }

  g_timeout_add(self->delay, UpdateUI, self);
  self->dataUpdateThread = std::thread(&Application::UpdateData, self);
}

void Application::UpdateData() {
  while (true) {
    btManager.updateDevList();
    stat.UpdateData();
    tempManager.update();
    paManager.updateDevices();
    // mprisManager.GetPlayerInfo(); Might Not be needed??? TODO for now i suppose
    brtManager.update();
    wifiManager.update();

    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
  }
}

gboolean Application::UpdateUI(gpointer data) {
  Application *self = static_cast<Application *>(data);

  self->btWindow.update();
  self->notifWindow.update();
  self->mprisWindow.update();
  self->brtWindow.update();
  self->paWindow.update();
  self->wifiWindow.update();

  for (auto &window : self->mainWindows) {
    window->update();
  }

  return true;
}
