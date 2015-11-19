// Current state of world:
struct world_state {
	unsigned int zoom;
	unsigned int size;
	struct center center;
};

struct world {
	const float *(*matrix)		(void);
	void (*move)			(struct world_state *state);
	void (*project)			(const struct world_state *state, float *vertex, float *normal, const float lat, const float lon);
	void (*zoom)			(const struct world_state *state);
	void (*center_restrict_tile)	(struct world_state *state);
	void (*center_restrict_latlon)	(struct world_state *state);
};

// Min and max longitude:
#define LON_MIN -M_PI
#define LON_MAX  M_PI

// Min and max latitude = atan(sinh(M_PI)):
#define LAT_MIN -1.484422229745332444395
#define LAT_MAX  1.484422229745332444395
