void camera_tilt (const int dy);
void camera_rotate (const int dx);
void camera_setup (const int screen_wd, const int screen_ht);
bool camera_is_tilted (void);
bool camera_is_rotated (void);
bool camera_visible_quad (const vec4f a, const vec4f b, const vec4f c, const vec4f d);
