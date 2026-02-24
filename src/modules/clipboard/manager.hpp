#pragma once

#include <wayland-client-protocol.h>
#include "../../wayland/ext-data-control-v1.h"
#include "../../app/context.hpp"
#include "gdk/gdk.h"

class ClipboardManager {
    wl_display *dp;
    wl_registry *registry;
    wl_seat *seat;
    ext_data_control_manager_v1 *dataCtrlMgr;
    ext_data_control_source_v1 *dataSrc;
    ext_data_control_device_v1 *dataDev;
    
    AppContext *ctx;
    
    
    static void regHandler(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
    static void regRemover(void *data, wl_registry *registry, uint32_t name);
    static constexpr wl_registry_listener regListener = {
        .global = regHandler,
        .global_remove = regRemover
    };
    
    static void dataSrcSend(void *data, ext_data_control_source_v1 *source, const char *mime_type, int32_t fd);
    static void dataSrcCancelled(void *data, ext_data_control_source_v1 *source);
    static constexpr ext_data_control_source_v1_listener dataSrcListener = {
        .send = dataSrcSend,
        .cancelled = dataSrcCancelled
    };
    
    
    
    public:
        bool error = false;
        ClipboardManager(AppContext *ctx);
        void init (GdkDisplay *gdkDp);
        ~ClipboardManager();

};