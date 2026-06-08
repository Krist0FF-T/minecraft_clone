#pragma once

#include <cstdint>
#include <array>

#include <raylib.h>

static const int BLOCK_TYPE_COUNT = 22;
static const int TEXTURE_COUNT = 23;

static const char *TEXTURE_NAMES[TEXTURE_COUNT]{
    "grass_top",  "grass_side",  "dirt",      "sand",        "stone",
    "brick",      "stone_bricks", "log_top",   "log_side",    "leaf",
    "wool_red",   "wool_lime",   "wool_blue", "wool_orange", "wool_yellow",
    "wool_lblue", "wool_black",  "wool_gray", "wool_white",  "cobblestone",
    "slime",      "planks",      "bookshelf",
};

// texture names enum
enum tne {
    tne_grass_top = 0,
    tne_grass_side,
    tne_dirt,
    tne_sand,
    tne_stone,
    tne_brick,
    tne_stone_bricks,
    tne_log_top,
    tne_log_side,
    tne_leaf,
    tne_wool_red,
    tne_wool_lime,
    tne_wool_blue,
    tne_wool_orange,
    tne_wool_yellow,
    tne_wool_lblue,
    tne_wool_black,
    tne_wool_gray,
    tne_wool_white,
    tne_cobblestone,
    tne_slime,
    tne_planks,
    tne_bookshelf,
};

enum biome { biome_forest = 0, biome_desert };

// block types
enum class BlockType : std::uint8_t {
    Air,
    Grass,
    Dirt,
    Sand,
    Stone,
    Brick,
    StoneBrick,
    Log,
    Leaf,
    WoolRed,
    WoolLime,
    WoolBlue,
    WoolOrange,
    WoolYellow,
    WoolLightBlue,
    WoolBlack,
    WoolGray,
    WoolWhite,
    Cobblestone,
    Slime,
    Planks,
    Bookshelf,
};

struct BlockData {
    const char *name;
    std::array<int, 3> sides;
    bool flippable, falling, translucent;
};

// clang-format off
static const std::array<BlockData, BLOCK_TYPE_COUNT> blockData {
    (BlockData){
        "Air",
        {tne_grass_top, tne_grass_side, tne_dirt},
        true, false, false
    },
    (BlockData){
        "Grass",
        {tne_grass_top, tne_grass_side, tne_dirt},
        true, false, false
    },

    (BlockData){
        "Dirt",
        {tne_dirt, tne_dirt, tne_dirt},
        true, false, false
    },

    (BlockData){
        "Sand",
        {tne_sand, tne_sand, tne_sand},
        true, true, false
    },

    (BlockData){
        "Stone",
        {tne_stone, tne_stone, tne_stone},
        true, false, false
    },

    (BlockData){
        "Brick",
        {tne_brick, tne_brick, tne_brick},
        false, false, false
    },

    (BlockData){
        "Stone bricks",
        {tne_stone_bricks,tne_stone_bricks,tne_stone_bricks},
        0,0,0
    },

    (BlockData){
        "Log",
        {tne_log_top, tne_log_side, tne_log_top},
        true, false, false
    },

    (BlockData){
        "Leaf",
        {tne_leaf, tne_leaf, tne_leaf},
        true, false, true
    },

    // ++ Wools

    (BlockData){
        "Red wool",
        {tne_wool_red, tne_wool_red, tne_wool_red},
        false, false, false
    },

    (BlockData){
        "Lime wool",
        {tne_wool_lime, tne_wool_lime, tne_wool_lime},
        false, false, false
    },

    (BlockData){
        "Blue wool",
        {tne_wool_blue, tne_wool_blue, tne_wool_blue},
        false, false, false
    },

    (BlockData) {
        "Orange wool",
        {tne_wool_orange,tne_wool_orange,tne_wool_orange},
        false,false,false
    },

    (BlockData){
        "Yellow wool",
        {tne_wool_yellow, tne_wool_yellow, tne_wool_yellow},
        false, false, false
    },

    (BlockData){
        "Light blue wool",
        {tne_wool_lblue, tne_wool_lblue, tne_wool_lblue},
        false, false, false
    },

    (BlockData){
        "Black wool",
        {tne_wool_black, tne_wool_black, tne_wool_black},
        false, false, false
    },

    (BlockData){
        "Gray wool",
        {tne_wool_gray, tne_wool_gray, tne_wool_gray},
        false, false, false
    },

    (BlockData){
        "White wool",
        {tne_wool_white, tne_wool_white, tne_wool_white},
        false, false, false
    },

    // -- Wools

    (BlockData) {
        "Cobblestone",
        {tne_cobblestone,tne_cobblestone,tne_cobblestone},
        false,false,false
    },

    (BlockData){
        "Slime",
        {tne_slime, tne_slime, tne_slime},
        true, false, false
    },

    (BlockData){
        "Planks",
        {tne_planks,tne_planks,tne_planks},
        false,false,false
    },

    (BlockData){
        "Bookshelf",
        {tne_planks,tne_bookshelf,tne_planks},
        false,false,false
    },

};
