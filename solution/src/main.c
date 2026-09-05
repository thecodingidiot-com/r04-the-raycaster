#include <snes.h>
#include "camera.h"
#include "map.h"
#include "render.h"

/*
** One byte of angle, so a "turn speed" is a number of 256ths of a
** full circle. 12 is about seventeen degrees per frame -- very
** coarse, and deliberately so: at roughly one frame every two
** seconds there is no point spreading a fine step across frames that
** arrive twice a minute. A quarter turn takes five frames this way
** instead of sixteen. The step sizes here are set by the measured
** frame rate, not by taste.
*/
# define TURN_SPEED 12
# define MOVE_SPEED FIX(0.35)

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
