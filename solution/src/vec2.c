#include "vec2.h"

t_vec2  vec2_add(t_vec2 a, t_vec2 b)
{
    t_vec2  r;

    r.x = a.x + b.x;
    r.y = a.y + b.y;
    return (r);
}

t_vec2  vec2_sub(t_vec2 a, t_vec2 b)
{
    t_vec2  r;

    r.x = a.x - b.x;
    r.y = a.y - b.y;
    return (r);
}

t_vec2  vec2_scale(t_vec2 a, t_fix s)
{
    t_vec2  r;

    r.x = fix_mul(a.x, s);
    r.y = fix_mul(a.y, s);
    return (r);
}

t_fix   vec2_dot(t_vec2 a, t_vec2 b)
{
    return (fix_mul(a.x, b.x) + fix_mul(a.y, b.y));
}
