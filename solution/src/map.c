#include "map.h"
#include "fixed.h"

extern const char   *g_map_template[10];
extern const int    g_map_rows;
extern const int    g_map_cols;

static void find_start(t_map *map)
{
    int             y;
    int             x;
    char            c;

    y = 0;
    while (y < map->rows) {
        x = 0;
        while (x < map->cols) {
            c = map->grid[y][x];
            if (c == 'N' || c == 'S' || c == 'E' || c == 'W') {
                map->start_pos.x = fix_from_int(x) + FIX(0.5);
                map->start_pos.y = fix_from_int(y) + FIX(0.5);
                if (c == 'E')
                    map->start_angle = 0;
                else if (c == 'S')
                    map->start_angle = 64;
                else if (c == 'W')
                    map->start_angle = 128;
                else
                    map->start_angle = 192;
                map->grid[y][x] = '0';
                return ;
            }
            x++;
        }
        y++;
    }
}

void    map_load(t_map *map)
{
    int y;
    int x;

    y = 0;
    while (y < g_map_rows) {
        x = 0;
        while (g_map_template[y][x]) {
            map->grid[y][x] = g_map_template[y][x];
            x++;
        }
        map->grid[y][x] = '\0';
        y++;
    }
    map->rows = g_map_rows;
    map->cols = g_map_cols;
    find_start(map);
}

int map_is_wall(t_map const *map, int x, int y)
{
    if (x < 0 || x >= map->cols || y < 0 || y >= map->rows)
        return (1);
    return (map->grid[y][x] != '0');
}
