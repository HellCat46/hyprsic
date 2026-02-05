#include "window.hpp"
#include "../utils/helper_func.hpp"
#include "dbus/dbus.h"
#include "gdk/gdk.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk-layer-shell.h"
#include "gtk/gtk.h"
#include <ctime>
#include <memory>

#define TAG "MainWindow"

Window::Window(AppContext *ctx, MprisManager *mprisMgr,
               ScreenSaverManager *scrnsavrMgr,
               NotificationManager *notifInstance, BluetoothManager *btMgr,
               HyprWSManager *hyprMgr, StatusNotifierManager *snManager,
               Stats *stat, Memory *mem, SysLoad *load, BatteryInfo *battery, PulseAudioManager* paMgr)
    : mprisModule(ctx, mprisMgr), hyprModule(ctx, hyprMgr),
      scrnsavrModule(ctx, scrnsavrMgr), btModule(ctx, btMgr),
      notifModule(ctx, notifInstance), snModule(ctx, snManager),
      sysinfoModule(stat, mem, load, battery), paModule(paMgr, &ctx->logger) {}

MainWindow::MainWindow()
    : notifManager(&ctx), btManager(&ctx), mprisManager(&ctx),
      scrnsavrManager(&ctx), hyprInstance(&ctx.logger), snManager(&ctx),
      load(&ctx.logger), mem(&ctx.logger), stat(&ctx.logger),
      battery(&ctx.logger), paManager(&ctx.logger), wifiManager(&ctx) {

  btManager.setup();
  hyprInstance.liveEventListener();
  ssnDBusThread = std::thread(&MainWindow::captureSessionDBus, this);

  app = gtk_application_new("com.hellcat.hyprsic", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), this);
  g_timeout_add(5000, UpdateData, this);
}

void MainWindow::RunApp() {
  int status = g_application_run(G_APPLICATION(app), 0, nullptr);
  g_object_unref(app);
}

void MainWindow::activate(GtkApplication *app, gpointer user_data) {
  MainWindow *self = static_cast<MainWindow *>(user_data);
  GdkDisplay *display = gdk_display_get_default();
  self->ctx.initUpdateWindow();
  
  int mCount = gdk_display_get_n_monitors(display);
  for (int i = 0; i < mCount; i++) {
    std::unique_ptr<Window> winInstance = std::unique_ptr<Window>(
        new Window(&self->ctx, &self->mprisManager, &self->scrnsavrManager,
                   &self->notifManager, &self->btManager, &self->hyprInstance,
                   &self->snManager, &self->stat, &self->mem, &self->load,
                   &self->battery, &self->paManager));
    GdkMonitor *monitor = gdk_display_get_monitor(display, i);

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

    GtkWidget *main_box = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(main_box), TRUE);
    gtk_container_add(GTK_CONTAINER(winInstance->window), main_box);

    // Right Box to Show System Stats
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_grid_attach(GTK_GRID(main_box), right_box, 3, 0, 2, 1);
    gtk_widget_set_hexpand(right_box, TRUE);
    
    GtkWidget *rightGrid = gtk_grid_new();
    gtk_box_pack_end(GTK_BOX(right_box), rightGrid, FALSE, FALSE, 0);
    gtk_grid_set_column_spacing(GTK_GRID(rightGrid), 10);
    gtk_widget_set_margin_end(right_box, 5);

    winInstance->hyprModule.setup(main_box);

    winInstance->mprisModule.setup(main_box);

    winInstance->sysinfoModule.setup(rightGrid);
    winInstance->notifModule.setup(rightGrid);
    winInstance->btModule.setupBT(rightGrid);
    winInstance->scrnsavrModule.setup(rightGrid);
    winInstance->snModule.setup(rightGrid);
    winInstance->paModule.setup(rightGrid);

    gtk_widget_show_all(winInstance->window);
    self->mainWindows.push_back(std::move(winInstance));
  }
}

gboolean MainWindow::UpdateData(gpointer data) {
  MainWindow *self = static_cast<MainWindow *>(data);

  self->btManager.getDeviceList();
  for (auto &window : self->mainWindows) {
    window->sysinfoModule.update();
    window->mprisModule.update();
    window->notifModule.update();
    window->btModule.updateBTList();
    window->snModule.update();
    window->paModule.update();
  }
  self->stat.UpdateData();
  self->paManager.getDevices();
  
  return true;
}

void MainWindow::captureSessionDBus() {
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
      notifManager.handleDbusMessage(msg, NotificationModule::showNotification);
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
