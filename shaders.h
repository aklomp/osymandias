#ifndef SHADERS_H
#define SHADERS_H

void shaders_init (void);
void shader_use_bkgd (const float cur_maxsize, const int offs_x, const int offs_y);
void shader_use_cursor (const float cur_halfwd, const float cur_halfht);
void shader_use_tile (const int cur_offs_x, const int cur_offs_y);

#endif	/* SHADERS_H */
