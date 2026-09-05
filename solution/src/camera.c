#include "camera.h"
#include "trig.h"

static void camera_rebuild_axes(t_camera *cam)
{
    cam->forward.x = g_cos_table[cam->angle];
    cam->forward.y = g_sin_table[cam->angle];
    cam->right.x = cam->forward.y;
    cam->right.y = -cam->forward.x;
}

void    camera_init(t_camera *cam, t_fix x, t_fix y, unsigned char angle)
{
    cam->pos.x = x;
    cam->pos.y = y;
    cam->angle = angle;
    camera_rebuild_axes(cam);
}

void    camera_turn(t_camera *cam, signed char delta_angle)
{
    cam->angle = (unsigned char)(cam->angle + delta_angle);
    camera_rebuild_axes(cam);
}

void    camera_move(t_camera *cam, t_fix delta_forward)
{
    cam->pos = vec2_add(cam->pos, vec2_scale(cam->forward, delta_forward));
}
