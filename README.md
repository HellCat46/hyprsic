# Hyprsic (Work In Progress)
A Status Bar for Hyprland inspired by Simplicity of i3Bar (Not Anymore i suppose).

## Features
- [x] System Information Display
- [x] Hyprland IPC Integration
- [x] PulseAudio Integration
- [x] Bluetooth Frontend
- [x] Tooltip Support
- [x] Multi-monitor Support
- [x] System Tray Integration
- [x] Playback Showcase & Controls
- [x] Input and Output Device Management
- [x] Notification Manager Integration
- [x] ScreenSaver Integration
- [x] Power Management Integration (Battery, etc) (Cancelled)
- [x] GPU Stats Module (Cancelled)
- [x] Brightness Control Integration  
- [ ] CLI to perform some Actions using calling commands. Could be used with Shortcuts by configuring them in Hyprland Config.
- [ ] Config to control which modules load & their Order in the Bar UI
- [ ] Encrypt Notification Contents (Using libsodium)
- [ ] Clipboard Manager Integration (Text, Image, Videos, Other Files, etc)

## Build
### For Nix Systems
```shell
nix-shell
./build.sh
```
### For Non-Nix Systems
**Dependencies**
```text
wayland
libpulse
dbus-cpp
jsoncpp
```

## Credits
- Icons from [Material Design Icons](https://fonts.google.com/icons) and [Lucide](https://lucide.dev/)

## References
- [Hyprland IPC](https://wiki.hyprland.org/IPC/)
- [Socket Programming](https://www.linuxhowtos.org/C_C++/socket.htm)
- [CPP Reference](https://en.cppreference.com/w/)
- [PulseAudio Async API](https://freedesktop.org/software/pulseaudio/doxygen/async.html)
- [Dbus-1 Docs](https://dbus.freedesktop.org/doc/dbus-specification.html)
- [Desktop Notification Specs - Linux](https://specifications.freedesktop.org/notification/latest/)

[//]: # (Why were dbus-1 docs so hard to find ;-;)