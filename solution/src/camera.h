#ifndef CAMERA_H
# define CAMERA_H

# include "vec2.h"

/* Angle is one byte, 0..255 around a full circle -- there is no
** radians here, and no runtime trig at all: the 65816 has no
** floating point and this project carries no software sin/cos
** either, only a 256-entry fixed-point table (trig.c). A u8 wraps
** for free on overflow, so turning never needs an explicit mask the
** way r02's 1024-step u16 angle did. */
typedef struct s_camera
{
    t_vec2          pos;
    unsigned char   angle;
    t_vec2          forward;
    t_vec2          right;
}   t_camera;

void    camera_init(t_camera *cam, t_fix x, t_fix y, unsigned char angle);
void    camera_turn(t_camera *cam, signed char delta_angle);
void    camera_move(t_camera *cam, t_fix delta_forward);

#endif
