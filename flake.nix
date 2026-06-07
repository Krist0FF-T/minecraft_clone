{
  description = "A MineCraft clone";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs = { self, nixpkgs }: let
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; };
  in {
    devShells.${system}.default = pkgs.mkShell {
      packages = with pkgs; [
        meson ninja cmake pkg-config
        libx11 libxrandr libxinerama libxcursor libxi
        wayland wayland-scanner libxkbcommon
        libGL
        raylib
      ];

      LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath [
        pkgs.alsa-lib
        pkgs.wayland
        pkgs.libxkbcommon
      ];
    };
  };
}
