#include <snes.h>
#include "camera.h"
#include "map.h"
#include "render.h"

/*
** One byte of angle, so a "turn speed" is a number of 256ths of a
** full circle. 4 is a little over five degrees per frame -- coarse,
** but this renderer will not be producing many frames per second to
** spread a finer step across.
*/
# define TURN_SPEED 4
# define MOVE_SPEED FIX(0.12)

static void handle_input(t_camera *cam, u16 pad)
{
    if (pad & KEY_LEFT)
        camera_turn(cam, -TURN_SPEED);
    if (pad & KEY_RIGHT)
        camera_turn(cam, TURN_SPEED);
    if (pad & KEY_UP)
        camera_move(cam, MOVE_SPEED);
    if (pad & KEY_DOWN)
        camera_move(cam, -MOVE_SPEED);
}

int main(void)
{
    t_map       map;
    t_camera    cam;

    map_load(&map);
    camera_init(&cam, map.start_pos.x, map.start_pos.y, map.start_angle);
    render_init();
    while (1) {
        handle_input(&cam, padsCurrent(0));
        render_frame(&cam, &map);
    }
    return (0);
}
