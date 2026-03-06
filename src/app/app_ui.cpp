#include "app.hpp"



Application::Application()
    : stat(&ctx.logger), mem(&ctx.logger), load(&ctx.logger), battery(&ctx),
      tempManager(&ctx), btManager(&ctx), btWindow(&ctx, &btManager),
      notifManager(&ctx), notifWindow(&ctx, &notifManager), mprisManager(&ctx),
      mprisWindow(&ctx, &mprisManager), scrnsavrManager(&ctx),
      hyprInstance(&ctx.logger), snManager(&ctx), paManager(&ctx.logger),
      paWindow(&ctx, &paManager), wifiManager(&ctx),
      wifiWindow(&ctx, &wifiManager), brtManager(&ctx),
      brtWindow(&ctx, &brtManager) {

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
  for (int i = 0; i < mCount; i++) {

    self->mainWindows.push_back(std::unique_ptr<Window>(new Window(
        &self->ctx, &self->hyprInstance, &self->snManager, &self->stat,
        &self->mem, &self->load, &self->battery, &self->tempManager,
        &self->scrnsavrManager, &self->mprisManager, &self->mprisWindow,
        &self->notifManager, &self->notifWindow, &self->btManager,
        &self->btWindow, &self->brtManager, &self->brtWindow, &self->paManager,
        &self->paWindow, &self->wifiManager, &self->wifiWindow)));
    GdkMonitor *monitor = gdk_display_get_monitor(display, i);

    auto &winInstance = self->mainWindows.back();

    winInstance->window = gtk_application_window_new(app);

    gtk_layer_init_for_window(GTK_WINDOW(winInstance->window));
    gtk_layer_set_layer(GTK_WINDOW(winInstance->window),
                        GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_monitor(GTK_WINDOW(winInstance->window), monitor);

    gtk_layer_set_anchor(GTK_WINDOW(winInstance->window),
                         GTK_LAYER_SHELL_EDGE_BOTTOM, true);
    gtk_layer_set_anchor(GTK_WINDOW(winInstance->window),
                         GTK_LAYER_SHELL_EDGE_LEFT, true);
    gtk_layer_set_anchor(GTK_WINDOW(winInstance->window),
                         GTK_LAYER_SHELL_EDGE_RIGHT, true);
    gtk_layer_set_exclusive_zone(GTK_WINDOW(winInstance->window), 25);
    gtk_widget_set_size_request(winInstance->window, -1, 25);

    GtkWidget *mainGrid = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(mainGrid), true);
    gtk_container_add(GTK_CONTAINER(winInstance->window), mainGrid);

    GtkWidget *wid = winInstance->hyprModule.setup(i);
    gtk_grid_attach(GTK_GRID(mainGrid), wid, 0, 0, 2, 1);

    wid = winInstance->mprisModule.setup();
    gtk_grid_attach(GTK_GRID(mainGrid), wid, 2, 0, 1, 1);

    // Right Box to Show System Stats
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_grid_attach(GTK_GRID(mainGrid), right_box, 3, 0, 3, 1);
    gtk_widget_set_hexpand(right_box, true);

    GtkWidget *rightGrid = gtk_grid_new();
    gtk_box_pack_end(GTK_BOX(right_box), rightGrid, false, false, 0);
    gtk_grid_set_column_spacing(GTK_GRID(rightGrid), 10);
    gtk_widget_set_margin_end(right_box, 5);

    // System Info Widgets
    auto wids = winInstance->sysinfoModule.setup();
    for (int i = 0; i < wids.size(); i++) {
      gtk_grid_attach(GTK_GRID(rightGrid), wids[i], i, 0, 1, 1);
    }

    int loc = wids.size();
    wid = winInstance->wifiModule.setup();
    gtk_grid_attach(GTK_GRID(rightGrid), wid, loc++, 0, 1, 1);

    wid = winInstance->brtModule.setup();
    gtk_grid_attach(GTK_GRID(rightGrid), wid, loc++, 0, 1, 1);

    wid = winInstance->notifModule.setup();
    gtk_grid_attach(GTK_GRID(rightGrid), wid, loc++, 0, 1, 1);

    wid = winInstance->btModule.setup();
    gtk_grid_attach(GTK_GRID(rightGrid), wid, loc++, 0, 1, 1);

    wid = winInstance->scrnsavrModule.setup();
    gtk_grid_attach(GTK_GRID(rightGrid), wid, loc++, 0, 1, 1);

    wids = winInstance->paModule.setup();
    for (int i = 0; i < wids.size(); i++) {
      gtk_grid_attach(GTK_GRID(rightGrid), wids[i], loc + i, 0, 1, 1);
    }

    loc += wids.size();
    wid = winInstance->snModule.setup();
    gtk_grid_attach(GTK_GRID(rightGrid), wid, loc, 0, 1, 1);

    gtk_widget_show_all(winInstance->window);
  }

  g_timeout_add(self->delay, UpdateUI, self);
  self->dataUpdateThread = std::thread(&Application::UpdateData, self);
}

void Application::UpdateData() {
  while (true) {
    btManager.getDeviceList();
    stat.UpdateData();
    tempManager.update();
    paManager.getDevices();
    mprisManager.GetPlayerInfo();
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
    window->sysinfoModule.update();
    window->mprisModule.update();
    window->snModule.update();
    window->paModule.update();
    window->brtModule.update();
    window->wifiModule.update();
  }

  return true;
}