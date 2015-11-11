enum worlds {
	WORLD_PLANAR,
	WORLD_SPHERICAL,
};

// Change current world:
void world_set (const enum worlds world);

// Get current world:
enum worlds world_get (void);

// Get world properties:
unsigned int world_get_size (void);
unsigned int world_get_zoom (void);

// Zoom one step in/out of world:
bool world_zoom_in (void);
bool world_zoom_out (void);

// Initialize worlds:
bool worlds_init (const unsigned int zoom);

// Destroy worlds:
void worlds_destroy (void);
