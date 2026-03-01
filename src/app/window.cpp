#include "window.hpp"
#include "../utils/helper_func.hpp"
#include "dbus/dbus.h"
#include "gdk/gdk.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk-layer-shell.h"
#include "gtk/gtk.h"
#include <cstring>
#include <ctime>
#include <memory>
#include <string_view>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

#define TAG "Application"

Window::Window(AppContext *ctx, MprisManager *mprisMgr,
               ScreenSaverManager *scrnsavrMgr,
               NotificationManager *notifInstance, BluetoothManager *btMgr,
               HyprWSManager *hyprMgr, StatusNotifierManager *snManager,
               Stats *stat, Memory *mem, SysLoad *load, BatteryInfo *battery,
               PulseAudioManager *paMgr, BrightnessManager *brtMgr,
               BluetoothWindow *btWindow, BrightnessWindow *brtWindow,
               MprisWindow *mprisWindow, NotificationWindow *notifWindow,
               PulseAudioWindow *paWindow)
    : mprisModule(ctx, mprisMgr, mprisWindow), hyprModule(ctx, hyprMgr),
      scrnsavrModule(ctx, scrnsavrMgr), btModule(ctx, btMgr, btWindow),
      notifModule(ctx, notifInstance, notifWindow), snModule(ctx, snManager),
      sysinfoModule(stat, mem, load, battery), paModule(paMgr, ctx, paWindow),
      brtModule(ctx, brtMgr, brtWindow) {}

Application::Application()
    : notifManager(&ctx), notifWindow(&ctx, &notifManager), btManager(&ctx),
      btWindow(&ctx, &btManager), mprisManager(&ctx),
      mprisWindow(&ctx, &mprisManager), scrnsavrManager(&ctx),
      hyprInstance(&ctx.logger), snManager(&ctx), load(&ctx.logger),
      mem(&ctx.logger), stat(&ctx.logger), battery(&ctx),
      paManager(&ctx.logger), paWindow(&ctx, &paManager), wifiManager(&ctx),
      clipboardManager(&ctx), brtManager(&ctx), brtWindow(&ctx, &brtManager) {

  app = gtk_application_new("com.hellcat.hyprsic", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), this);
}

void Application::Run(int argc, char **argv) {
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
}

void Application::activate(GtkApplication *app, gpointer user_data) {
  Application *self = static_cast<Application *>(user_data);
  GdkDisplay *display = gdk_display_get_default();

  self->btManager.setup();
  self->hyprInstance.liveEventListener();
  self->ssnDBusThread = std::thread(&Application::captureSessionDBus, self);
  self->cliIPCThread = std::thread(&Application::captureCLIIPC, self);

  self->ctx.initWindows();
  self->paWindow.setupIcons();
  self->paWindow.init();
  self->btWindow.init();
  self->mprisWindow.init();
  self->notifWindow.init();
  self->brtWindow.init();
  // clipboardManager.init(display);

  int mCount = gdk_display_get_n_monitors(display);
  for (int i = 0; i < mCount; i++) {

    self->mainWindows.push_back(std::unique_ptr<Window>(new Window(
        &self->ctx, &self->mprisManager, &self->scrnsavrManager,
        &self->notifManager, &self->btManager, &self->hyprInstance,
        &self->snManager, &self->stat, &self->mem, &self->load, &self->battery,
        &self->paManager, &self->brtManager, &self->btWindow, &self->brtWindow,
        &self->mprisWindow, &self->notifWindow, &self->paWindow)));
    GdkMonitor *monitor = gdk_display_get_monitor(display, i);

    auto &winInstance = self->mainWindows.back();

    winInstance->window = gtk_application_window_new(app);

    gtk_layer_init_for_window(GTK_WINDOW(winInstance->window));
    gtk_layer_set_layer(GTK_WINDOW(winInstance->window),
                        GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_monitor(GTK_WINDOW(winInstance->window), monitor);

    gtk_layer_set_anchor(GTK_WINDOW(winInstance->window),
                         GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(winInstance->window),
                         GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(winInstance->window),
                         GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_exclusive_zone(GTK_WINDOW(winInstance->window), 25);
    gtk_widget_set_size_request(winInstance->window, -1, 25);

    GtkWidget *mainGrid = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(mainGrid), TRUE);
    gtk_container_add(GTK_CONTAINER(winInstance->window), mainGrid);

    GtkWidget *wid = winInstance->hyprModule.setup(i);
    gtk_grid_attach(GTK_GRID(mainGrid), wid, 0, 0, 2, 1);

    wid = winInstance->mprisModule.setup();
    gtk_grid_attach(GTK_GRID(mainGrid), wid, 2, 0, 1, 1);

    // Right Box to Show System Stats
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_grid_attach(GTK_GRID(mainGrid), right_box, 3, 0, 2, 1);
    gtk_widget_set_hexpand(right_box, TRUE);

    GtkWidget *rightGrid = gtk_grid_new();
    gtk_box_pack_end(GTK_BOX(right_box), rightGrid, FALSE, FALSE, 0);
    gtk_grid_set_column_spacing(GTK_GRID(rightGrid), 10);
    gtk_widget_set_margin_end(right_box, 5);

    // System Info Widgets
    auto wids = winInstance->sysinfoModule.setup();
    for (int i = 0; i < wids.size(); i++) {
      gtk_grid_attach(GTK_GRID(rightGrid), wids[i], i, 0, 1, 1);
    }

    int loc = wids.size();
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
    paManager.getDevices();
    mprisManager.GetPlayerInfo();
    brtManager.update();

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

  for (auto &window : self->mainWindows) {
    window->sysinfoModule.update();
    window->mprisModule.update();
    window->snModule.update();
    window->paModule.update();
    window->brtModule.update();
  }

  return true;
}

void Application::captureSessionDBus() {
  notifManager.setupDBus();
  snManager.setupDBus();

  DBusMessage *msg;
  while (1) {
    if (!dbus_connection_read_write_dispatch(ctx.dbus.ssnConn, 100)) {
      ctx.logger.LogError(
          TAG, "Connection Closed while Waiting for Notification Messages");
      return;
    }

    msg = dbus_connection_pop_message(ctx.dbus.ssnConn);
    if (!msg) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    const char *interface = dbus_message_get_interface(msg);
    const char *path = dbus_message_get_path(msg);
    const char *member = dbus_message_get_member(msg);

    if (HelperFunc::saferStrCmp(interface, "org.freedesktop.Notifications")) {
      notifManager.handleDbusMessage(msg, NotificationWindow::showNotification);
    } else if (HelperFunc::saferStrCmp(interface, "org.freedesktop.DBus") &&
               HelperFunc::saferStrCmp(member, "NameOwnerChanged") &&
               HelperFunc::saferStrCmp(path, "/org/freedesktop/DBus")) {

      DBusMessageIter args;
      dbus_message_iter_init(msg, &args);

      const char *name, *oldOwner, *newOwner;
      dbus_message_iter_get_basic(&args, &name);
      dbus_message_iter_next(&args);
      dbus_message_iter_get_basic(&args, &oldOwner);
      dbus_message_iter_next(&args);
      dbus_message_iter_get_basic(&args, &newOwner);

      if (HelperFunc::saferStrNCmp(name, "org.mpris.MediaPlayer2", 22)) {
        if (std::strlen(newOwner) != 0) {
          mprisManager.addPlayer(name);
        } else {
          mprisManager.removePlayer(name);
        }
      }

      snManager.handleNameOwnerChangedSignal(msg, name, newOwner);
    }
    {
      snManager.handleDbusMessage(msg);
    }

    dbus_message_unref(msg);
  }
}

void Application::captureCLIIPC() {
  int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  sockaddr_un addr;

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  std::string socketPath = "/tmp/hyprsic_ipc.sock";
  strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

  unlink(addr.sun_path);
  int ret = bind(sockfd, (sockaddr *)&addr, sizeof(addr));
  if (ret == -1) {
    ctx.logger.LogError(TAG, "Failed to bind socket: " +
                                 std::string(strerror(errno)));
    return;
  }

  listen(sockfd, 1);

  while (true) {
    int clientFd = accept(sockfd, nullptr, nullptr);
    if (clientFd == -1) {
      ctx.logger.LogError(TAG, "Failed to accept connection: " +
                                   std::string(strerror(errno)));
      continue;
    }

    char buff[256];
    memset(buff, 0, sizeof(buff));

    ssize_t bytesRead = read(clientFd, buff, sizeof(buff) - 1);
    if (bytesRead <= 0) {
      ctx.logger.LogError(TAG, "Failed to read from socket: " +
                                   std::string(strerror(errno)));
      close(clientFd);
      continue;
    }

    std::string_view cmd(buff);
    
    ctx.logger.LogInfo(TAG, "Received IPC Command: " + std::string(cmd));

    size_t spacePos = cmd.find(' ');
    if (spacePos == std::string_view::npos) {
      ctx.logger.LogError(TAG, "Invalid command format received: " +
                                   std::string(cmd));
      close(clientFd);
      continue;
    }

    
    std::string_view action = cmd.substr(0, spacePos);
    cmd = cmd.substr(spacePos + 1);

    std::vector<std::string_view> args;
    while (true) {
      spacePos = cmd.find(' ');
      if (spacePos == std::string_view::npos) {
        break;
      }

      args.push_back(cmd.substr(0, spacePos));
      cmd = cmd.substr(spacePos + 1);
    }

    if (args.size() == 0) {
      ctx.logger.LogError(TAG, "No arguments provided for command: " +
                                   std::string(action));
      close(clientFd);
      continue;
    }



          handleActions(action, args);
 
    close(clientFd);
  }
}

void Application::handleActions(std::string_view action,
                                std::vector<std::string_view> args) {
  if (action == "toggle-view") {
    handleIPCAction(args[0]);
  }
}

void Application::handleIPCAction(std::string_view module) {
  if (module == "pulseaudio") {
      
    ctx.showCtrlWindow("pulseaudio", 400, -1);
  } else if (module == "bluetooth") {
      
    ctx.showCtrlWindow("bluetooth", 400, 200);
  } else if (module == "notifications") {
      
    ctx.showCtrlWindow("notifications", 420, 400);
  } else if (module == "brightness") {
      
    ctx.showCtrlWindow("brightness", 340, 70);
  }
}
