#include "map.h"

/*
** The same layout r03's fixtures/map1.map describes -- a cartridge
** has no filesystem, so there is no text file to read at runtime.
** This is that same grid, compiled straight into the ROM as a plain
** array of strings instead, the same trade g02b's/r02's own compiled-
** in data made for the same reason.
**
** One character differs from r03's file, deliberately: the start
** marker is S where r03 has N. Same cell, opposite facing. r03 spawns
** looking north at a flat wall 1.5 cells away, which is fine when
** every wall carries a texture -- and useless here, where they do not.
** Facing north from this cell is the one view in the whole map with
** no depth variation at all: every column reports the same distance,
** so the screen is a single flat band. Correct, and completely
** uninformative as the first thing anyone sees. Facing south puts a
** corner in shot instead.
*/
const char  *g_map_template[10] =
{
    "1111111111111",
    "1000000000001",
    "1011111011101",
    "1000000000001",
    "1001S10000001",
    "1000010111101",
    "1000010000001",
    "1000011111101",
    "1000000000001",
    "1111111111111",
};

const int   g_map_rows = 10;
const int   g_map_cols = 13;
