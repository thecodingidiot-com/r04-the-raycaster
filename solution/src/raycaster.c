#include "raycaster.h"

/*
** No angle, no trig table lookup here at all -- ray_dir is built from
** two vectors already sitting in t_camera: forward and right, scaled
** by PLANE_SCALE to set the field of view. column_x walks from -1
** (left edge of screen) to +1 (right edge), same as r03.
*/
static t_vec2   ray_direction(t_camera const *cam, int column)
{
    t_fix   column_x;
    t_vec2  plane;
    t_vec2  dir;

    /*
    ** Not fix_div(FIX(2 * column), FIX(WINDOW_W)). That reads more
    ** naturally and is wrong here: fix_div shifts its numerator left
    ** by 12 before dividing, so FIX(254) << 12 reaches 4.26 billion --
    ** past the top of a signed 32-bit integer, on a value that never
    ** needed to be that large. Shifting the plain column index by 13
    ** (one for the doubling, twelve for the fraction) computes the
    ** identical ratio with a peak intermediate of about a million.
    */
    column_x = (((t_fix)column << (FIX_FRAC_BITS + 1)) / WINDOW_W) - FIX(1);
    plane = vec2_scale(cam->right, PLANE_SCALE);
    dir = vec2_add(cam->forward, vec2_scale(plane, column_x));
    return (dir);
}

static t_fix    safe_inv(t_fix v)
{
    t_fix   inv;

    /*
    ** A ray running exactly along a grid axis has a zero component,
    ** and 1/0 needs a stand-in "never reached" distance. FIX(64), not
    ** something grander like FIX(1000), because this value is later
    ** multiplied by a within-cell fraction of up to FIX(1.0): at
    ** FIX(1000) that product is 1.7e10 and silently wraps a 32-bit
    ** result, at FIX(64) it peaks near 1.07e9 and fits. 64 is still
    ** far past the far corner of a 13x10 map, so the axis holding it
    ** can never win the DDA's comparison before a wall is hit.
    **
    ** v == 0 is only the loudest case of the same problem. A ray very
    ** nearly along an axis has a component of 1 or 2, and 1/v climbs
    ** just as far past what the multiply can hold -- v == 1 alone
    ** yields FIX(4096). So the ceiling is applied to the result, not
    ** to the special case, and the special case is only there because
    ** the division itself cannot be asked about zero.
    */
    if (v == 0)
        return (FIX(64));
    inv = fix_div(FIX(1), v);
    if (inv < 0)
        inv = -inv;
    if (inv > FIX(64))
        inv = FIX(64);
    return (inv);
}

/*
** Same DDA r03 uses, same reason: step from one grid line to the
** next instead of tiny constant steps, so a thin wall can never be
** tunnelled through. Only the arithmetic changed -- t_fix instead of
** float, fix_mul/fix_div instead of the FPU.
*/
t_hit   raycaster_cast(t_camera const *cam, t_map const *map, int column)
{
    t_vec2  dir;
    t_vec2  delta;
    t_vec2  side_dist;
    int     map_x;
    int     map_y;
    int     step_x;
    int     step_y;
    t_hit   hit;

    dir = ray_direction(cam, column);
    delta.x = safe_inv(dir.x);
    delta.y = safe_inv(dir.y);
    map_x = fix_int(cam->pos.x);
    map_y = fix_int(cam->pos.y);
    if (dir.x < 0) {
        step_x = -1;
        side_dist.x = fix_mul(cam->pos.x - fix_from_int(map_x), delta.x);
    } else {
        step_x = 1;
        side_dist.x = fix_mul(fix_from_int(map_x + 1) - cam->pos.x, delta.x);
    }
    if (dir.y < 0) {
        step_y = -1;
        side_dist.y = fix_mul(cam->pos.y - fix_from_int(map_y), delta.y);
    } else {
        step_y = 1;
        side_dist.y = fix_mul(fix_from_int(map_y + 1) - cam->pos.y, delta.y);
    }
    hit.side = 0;
    while (1) {
        if (side_dist.x < side_dist.y) {
            side_dist.x += delta.x;
            map_x += step_x;
            hit.side = 0;
        } else {
            side_dist.y += delta.y;
            map_y += step_y;
            hit.side = 1;
        }
        if (map_is_wall(map, map_x, map_y))
            break ;
    }
    if (hit.side == 0)
        hit.perp_dist = side_dist.x - delta.x;
    else
        hit.perp_dist = side_dist.y - delta.y;
    return (hit);
}
