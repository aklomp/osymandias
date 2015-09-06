#ifndef SHADERS_H
#define SHADERS_H

bool shaders_init (void);
void shader_use_cursor (const float cur_halfwd, const float cur_halfht);
void shader_use_tile (const int cur_offs_x, const int cur_offs_y, const int texture_offs_x, const int texture_offs_y, const int zoomfactor);

#endif	/* SHADERS_H */
