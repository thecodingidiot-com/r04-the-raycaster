#ifndef RAYCASTER_H
# define RAYCASTER_H

# include "vec2.h"
# include "camera.h"
# include "map.h"

/*
** 128x112, where r03 rendered 640x480 and r02 320x224.
**
** The width is not a budget decision, it is the hardware's ceiling.
** This renderer's output reaches the screen through Mode 7, and Mode 7
** addresses exactly 256 tiles of 64 pixels -- 16384 unique pixels in
** total, for the entire background. 128x128 is therefore the largest
** framebuffer the mode can express at all, no matter how much RAM or
** ROM the cartridge carries. See render.c.
**
** 112 rather than 128 rows is the one free choice here: 128x112
** doubles to exactly 256x224, so every framebuffer pixel becomes a
** clean 2x2 block on screen. Stretching a square 128x128 to the same
** screen would need 1.75x vertically, and rows would land unevenly.
*/
# define WINDOW_W       128
# define WINDOW_H       112
# define PLANE_SCALE    FIX(0.66)

/*
** No tex_id/wall_x here the way r03 had them -- no per-pixel texture
** sampling this chapter, see render.c's own comment on why. `side`
** alone drives shading: a real, if coarser, stand-in for the same
** north/south-vs-east/west light difference real Wolfenstein-style
** renderers use.
*/
typedef struct s_hit
{
    t_fix   perp_dist;
    int     side;
}   t_hit;

t_hit   raycaster_cast(t_camera const *cam, t_map const *map, int column);

#endif
