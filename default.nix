{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation {
  pname = "hyprsic";
  version = "0.0.1";

  src = ./.;

  nativeBuildInputs = with pkgs; [
    pkg-config
    gcc
    wayland-scanner
  ];

  buildInputs = with pkgs; [
    wayland
    wayland-protocols
    jsoncpp
    librsvg
    dbus-cpp
    dbus.dev
    libpulseaudio
    libxkbcommon
    cairo
    pango
    systemd
    cudaPackages.cuda_nvml_dev
  ];

  meta = with pkgs.lib; {
    description = "A status bar for Hyprland";
    license = licenses.mit;
    platforms = platforms.linux;
  };
}