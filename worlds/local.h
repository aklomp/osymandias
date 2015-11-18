// Current state of world:
struct world_state {
	unsigned int zoom;
	unsigned int size;
	struct center center;
};

struct world {
	const float *(*matrix)	(void);
	void (*move)		(const struct world_state *state);
	void (*project)		(const struct world_state *state, float *vertex, float *normal, const float lat, const float lon);
	void (*zoom)		(const struct world_state *state);
};
