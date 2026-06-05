
# MineCraft clone
A simple MineCraft clone written in C++ using [Raylib](https://www.raylib.com/).

# Features
- loading and saving worlds
- placing and breaking blocks
- throwing blocks
- commands (fill, replace, tp, reach, speed, rand_replace)
- ambient occlusion (smooth lighting)
- zooming (animated)
- keystrokes (in the top right corner)

# Controls
- WASD to move around
- space to jump
- F to toggle noclip
- HJKL to look around (like in Vim)
    - hold Alt to change the speed
- C to zoom
- V to throw the selected block
- U to break blocks
- O to place blocks
    - throws it if no block is selected
- ctrl+L to load a world
- ctrl+S to save the world

# Building
Ensure you have meson, ninja and cmake installed, then run

```bash
git clone https://github.com/Krist0FF-T/minecraft_clone.git
cd minecraft_clone
meson setup build
meson compile -C build
./build/game
```

