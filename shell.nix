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
  ];

  CMAKE_EXPORT_COMPILE_COMMANDS = "ON";

  # Environment variables
  shellHook = ''
    export PS1="\n[hyprsic:\w]$ "
  '';
}