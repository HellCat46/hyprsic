{
  description = "Hyprsic: A status bar for Hyprland";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      nixpkgs,
      flake-utils,
    }:
    
    flake-utils.lib.eachSystem [ "x86_64-linux" "aarch64-linux" ] (
      system:
      let
        pkgs = import nixpkgs { inherit system; };

        hyprsic-pkg = pkgs.callPackage ./default.nix { };
      in
      {
        packages.default = hyprsic-pkg;
        packages.hyprsic = hyprsic-pkg;

        apps.default = {
          type = "app";
          program = "${hyprsic-pkg}/bin/hyprsic";
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ hyprsic-pkg ];

          buildInputs = with pkgs; [
            gdb
            valgrind
            clang
            clang-tools
          ];

          shellHook = ''
            export PS1="\n[hyprsic-dev:\w]$ "
            export LC_ALL="en_US.UTF-8"
            export LANG="en_US.UTF-8"
            export LOCALE_ARCHIVE="${pkgs.glibcLocales}/lib/locale/locale-archive"
          '';
        };
      }
    );

}
