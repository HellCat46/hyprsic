#include "manager.hpp"
#include <iostream>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

ClipboardManager::ClipboardManager(){
    dp = wl_display_connect(nullptr);
    registry = wl_display_get_registry(dp);
    
    wl_registry_add_listener(registry, &registry_listener, this);
    wl_display_roundtrip(dp);
}
// Registry handler called:
// 	Interface:zwlr_data_control_manager_v1
// 	Version: 2
// 	Name: 38
// Registry handler called:
// 	Interface:ext_data_control_manager_v1
// 	Version: 1
// 	Name: 54
void ClipboardManager::registry_handler(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version){
    std::cout << "Registry handler called: \n\tInterface:" << interface << "\n\tVersion: "<<version << "\n\tName: "<<name<<std::endl;
    // std::cout << wl_data_device_manager.interface.name << std::endl;
    // wl_registry_bind(registry, name, &ext_data, version);
}

void ClipboardManager::registry_remover(void *data, wl_registry *registry, uint32_t name){
    std::cout << "Registry remover called: \n\tName: "<<name<<std::endl;
}

ClipboardManager::~ClipboardManager(){
    wl_registry_destroy(registry);
    wl_display_disconnect(dp);
}