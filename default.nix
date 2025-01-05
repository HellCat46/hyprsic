{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation {
  pname = "hyprsic";
  version = "0.0.1";

  src = ./.;

  nativeBuildInputs = with pkgs; [
    pkg-config
    cmake
    gcc
    wayland-scanner
  ];

  buildInputs = with pkgs; [
    wayland
    wayland-protocols
    jsoncpp
    libxkbcommon
    cairo
    pango
    systemd
  ];

  meta = with pkgs.lib; {
    description = "A status bar for Hyprland";
    license = licenses.mit;
    platforms = platforms.linux;
  };
}