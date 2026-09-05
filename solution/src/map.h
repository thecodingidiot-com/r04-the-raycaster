#ifndef MAP_H
# define MAP_H

# include "vec2.h"

# define MAP_MAX_ROWS   16
# define MAP_MAX_COLS   16

/* Palette indices, not RGB -- render.c's linear framebuffer is one
** byte per pixel straight into CGRAM slots main.c loads once. */
# define PAL_FLOOR      1
# define PAL_CEIL       2
# define PAL_WALL_NS    3
# define PAL_WALL_EW    4
# define PAL_METER      5

typedef struct s_map
{
    char    grid[MAP_MAX_ROWS][MAP_MAX_COLS + 1];
    int     rows;
    int     cols;
    t_vec2  start_pos;
    unsigned char   start_angle;
}   t_map;

/* No fopen, no filesystem: a cartridge has neither. map_load() copies
** the compiled-in template (map_data.c) into a mutable RAM grid and
** resolves its one N/S/E/W start marker, the same job r03's map_load()
** did against a real file. */
void    map_load(t_map *map);
int     map_is_wall(t_map const *map, int x, int y);

#endif
