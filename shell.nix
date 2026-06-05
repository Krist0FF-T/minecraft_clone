
let
    pkgs = import <nixpkgs> {};
in pkgs.mkShell {
    buildInputs = with pkgs; [
        raylib

        meson ninja cmake
        pkg-config

        libx11 libxrandr libxinerama libxcursor libxi

        # wayland wayland-protocols wayland-scanner libxkbcommon

        libGL

    ];
}
