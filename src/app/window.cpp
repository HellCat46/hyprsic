#include "window.hpp"
#include "modules/wifi/module.hpp"
#include <cstring>
#include <ctime>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define TAG "Application"

Window::Window(AppContext *ctx, HyprWSManager *hyprMgr,
               StatusNotifierManager *snManager, Stats *stat, Memory *mem,
               SysLoad *load, BatteryInfo *battery, TemperatureManager *tempMgr,
               ScreenSaverManager *scrnsavrMgr, MprisManager *mprisMgr,
               MprisWindow *mprisWindow, NotificationManager *notifInstance,
               NotificationWindow *notifWindow, BluetoothManager *btMgr,
               BluetoothWindow *btWindow, BrightnessManager *brtMgr,
               BrightnessWindow *brtWindow, PulseAudioManager *paMgr,
               PulseAudioWindow *paWindow, WifiManager *wifiMgr,
               WifiWindow *wifiWin)
    : sysinfoModule(ctx, stat, mem, load, battery, tempMgr),
      mprisModule(ctx, mprisMgr, mprisWindow), hyprModule(ctx, hyprMgr),
      scrnsavrModule(ctx, scrnsavrMgr), btModule(ctx, btMgr, btWindow),
      notifModule(ctx, notifInstance, notifWindow), snModule(ctx, snManager),
      paModule(paMgr, ctx, paWindow), brtModule(ctx, brtMgr, brtWindow), wifiModule(ctx, wifiMgr, wifiWin ){}
