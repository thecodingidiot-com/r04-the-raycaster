#ifndef RENDER_H
# define RENDER_H

# include "camera.h"
# include "map.h"

void    render_init(void);
void    render_frame(t_camera const *cam, t_map const *map);

#endif
