#ifndef VEC2_H
# define VEC2_H

# include "fixed.h"

typedef struct s_vec2
{
    t_fix   x;
    t_fix   y;
}   t_vec2;

t_vec2  vec2_add(t_vec2 a, t_vec2 b);
t_vec2  vec2_sub(t_vec2 a, t_vec2 b);
t_vec2  vec2_scale(t_vec2 a, t_fix s);
t_fix   vec2_dot(t_vec2 a, t_vec2 b);

#endif
