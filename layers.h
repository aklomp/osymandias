struct layer;

struct layer *layer_create (bool (*full_occlusion)(void), void (*paint)(void), void (*zoom)(void), void (*destroy)(void *), void *destroy_data);
bool layer_register (struct layer *l, int zindex);
void layers_destroy (void);
void layers_paint (void);
void layers_zoom (void);
