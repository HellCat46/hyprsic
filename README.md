# Hyprsic (Work In Progress)
A Status Bar for Hyprland inspired by Simplicity of i3Bar (Not Anymore i suppose).

## Features
- [x] System Information Display
- [x] Hyprland IPC Integration
- [x] PulseAudio Integration
- [x] Bluetooth Frontend
- [ ] Tooltip Support
- [x] Multi-monitor Support
- [ ] System Tray Integration
- [x] Playback Showcase & Controls
- [ ] Input and Output Device Management
- [ ] Config to control which modules load & their Order in the Bar UI
- [x] Notification Manager Integration
- [ ] Encrypt Notification Contents (Using libsodium)
- [ ] Clipboard Manager Integration (Text, Image, Videos, Other Files, etc)
- [x] ScreenSaver Integration
- [ ] Power Management Integration (Battery, Brightness, etc)
- [ ] GPU Stats Module

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

## References
- [Hyprland IPC](https://wiki.hyprland.org/IPC/)
- [Socket Programming](https://www.linuxhowtos.org/C_C++/socket.htm)
- [CPP Reference](https://en.cppreference.com/w/)
- [PulseAudio Async API](https://freedesktop.org/software/pulseaudio/doxygen/async.html)
- [Dbus-1 Docs](https://dbus.freedesktop.org/doc/api/html/group__DBusBus.html)
- [Desktop Notification Specs - Linux](https://specifications.freedesktop.org/notification/latest/)

[//]: # (Why were dbus-1 docs so hard to find ;-;)