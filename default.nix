{ pkgs ? import <nixpkgs> {}, lib ? pkgs.lib }:

pkgs.stdenv.mkDerivation {
  pname = "hyprsic";
  version = "0.1.0";

  src = ./.;

  nativeBuildInputs = with pkgs; [
    pkg-config
    meson
    ninja
    gcc
    wayland-scanner
  ];

  buildInputs = with pkgs; [
    wayland
    wayland-protocols
    jsoncpp
    dbus-cpp
    dbus.dev
    libpulseaudio
    libxkbcommon
    cairo
    pango
    systemd
    gtk3
    gtk-layer-shell
    sqlite
    sqlitecpp
  ];

  meta = {
    description = "A status bar for Hyprland";
    license = lib.licenses.mit;
    platforms = lib.platforms.linux;
  };
}