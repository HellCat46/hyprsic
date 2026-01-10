{ pkgs ? import <nixpkgs> {} }:

let
  project = import ./default.nix { inherit pkgs; };
in
pkgs.mkShell {
  inputsFrom = [ project ];

  # Additional development tools
  buildInputs = with pkgs; [
    gdb
    valgrind
    clang-tools
    dbus
    gtk3
    gtk-layer-shell
    meson
    sqlite
  ];

  CMAKE_EXPORT_COMPILE_COMMANDS = "ON";

  # Environment variables
  shellHook = ''
    export PS1="\n[hyprsic:\w]$ "
    export LC_ALL="en_US.UTF-8"
    export LANG="en_US.UTF-8"
    export LOCALE_ARCHIVE="${pkgs.glibcLocales}/lib/locale/locale-archive"
  '';
}
