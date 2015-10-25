bool camera_init (void);
void camera_destroy (void);
void camera_tilt (const float radians);
void camera_rotate (const float radians);
void camera_projection (const int screen_wd, const int screen_ht);
void camera_setup (void);
bool camera_is_tilted (void);
bool camera_is_rotated (void);
bool camera_visible_quad (const vec4f a, const vec4f b, const vec4f c, const vec4f d);
float camera_distance_squared (const struct vector *);
vec4f camera_distance_squared_quad (const vec4f x, const vec4f y, const vec4f z);
vec4f camera_distance_squared_quadedge (const vec4f x, const vec4f y, const vec4f z);
const float *camera_mat_viewproj (void);
