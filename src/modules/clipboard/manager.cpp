#include "manager.hpp"
#include "../../utils/helper_func.hpp"
#include "gdk/gdk.h"
#include "gdk/gdkwayland.h"
#include <iostream>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#define TAG "ClipboardManager"

ClipboardManager::ClipboardManager(AppContext *ctx) : ctx(ctx), registry(nullptr), dp(nullptr) {}

void ClipboardManager::init(GdkDisplay *gdkDp) {
  if (!GDK_IS_WAYLAND_DISPLAY(gdkDp)) {
    error = true;
    return;
  }

  dp = gdk_wayland_display_get_wl_display(gdkDp);
  registry = wl_display_get_registry(dp);

  wl_registry_add_listener(registry, &regListener, this);
  wl_display_roundtrip(dp);

  dataSrc = ext_data_control_manager_v1_create_data_source(dataCtrlMgr);
  dataDev = ext_data_control_manager_v1_get_data_device(dataCtrlMgr, seat);

  ext_data_control_source_v1_offer(dataSrc, "text/plain");
  ext_data_control_source_v1_offer(dataSrc, "text/plain;charset=utf-8");
  ext_data_control_source_v1_add_listener(dataSrc, &dataSrcListener, this);

  ext_data_control_device_v1_set_selection(dataDev, dataSrc);
  ext_data_control_device_v1_set_primary_selection(dataDev, dataSrc);
}

void ClipboardManager::regHandler(void *data, wl_registry *registry,
                                  uint32_t name, const char *interface,
                                  uint32_t version) {
  ClipboardManager *self = static_cast<ClipboardManager *>(data);

  if (HelperFunc::saferStrCmp(interface,
                              ext_data_control_manager_v1_interface.name)) {

    self->dataCtrlMgr =
        static_cast<ext_data_control_manager_v1 *>(wl_registry_bind(
            registry, name, &ext_data_control_manager_v1_interface, version));
    self->ctx->logger.LogDebug(TAG, "Data Control Manager Found in Registry");
  } else if (HelperFunc::saferStrCmp(interface, wl_seat_interface.name)) {

    self->seat = static_cast<wl_seat *>(
        wl_registry_bind(registry, name, &wl_seat_interface, version));
    self->ctx->logger.LogDebug(TAG, "Seat Found in Registry");
  }
}

void ClipboardManager::regRemover(void *data, wl_registry *registry,
                                  uint32_t name) {
  std::cout << "Registry remover called: \n\tName: " << name << std::endl;
}

void ClipboardManager::dataSrcSend(void *data,
                                   ext_data_control_source_v1 *source,
                                   const char *mime_type, int32_t fd) {
  ClipboardManager *self = static_cast<ClipboardManager *>(data);
  
  ssize_t wrote = write(fd, "Weeeeee Woooooo", 15);
  self->ctx->logger.LogDebug(TAG, "Data Source Send called. Wrote " + std::to_string(wrote) + " bytes to fd.");

  close(fd);
}

void ClipboardManager::dataSrcCancelled(void *data,
                                        ext_data_control_source_v1 *source) {
  ext_data_control_source_v1_destroy(source);
}

ClipboardManager::~ClipboardManager() {
  if(registry != nullptr) wl_registry_destroy(registry);
  if(dp != nullptr) wl_display_disconnect(dp);
}
