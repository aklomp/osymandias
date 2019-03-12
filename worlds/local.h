#pragma once

#include <vec/vec.h>

// Mark a position in time:
struct mark {
	struct coords coords;
	int64_t time;
};

// Current state of world:
struct world_state {
	unsigned int zoom;
	unsigned int size;
	struct coords center;
	struct {
		struct mark down;
		struct mark hold;
		struct mark free;
		struct coords speed;
		bool active;
	} autoscroll;
};

struct world {
	const float *(*matrix)		(void);
	const float *(*matrix_inverse)	(void);
	void (*move)			(const struct world_state *state);
	void (*project)			(const struct world_state *state, union vec *vertex, const float lat, const float lon);
	void (*zoom)			(const struct world_state *state);
	void (*center_restrict_tile)	(struct world_state *state);
	void (*center_restrict_latlon)	(struct world_state *state);
	bool (*timer_tick)		(struct world_state *state, int64_t usec);
};

// Clamp function:
#define clamp(var, min, max)			\
	do {					\
		if ((var) < (min))		\
			(var) = (min);		\
						\
		if ((var) > (max))		\
			(var) = (max);		\
	} while (0)

// Wraparound function:
#define wrap(var, min, max)			\
	do {					\
		if ((var) < (min))		\
			(var) += (max);		\
						\
		if ((var) > (max))		\
			(var) -= (max);		\
	} while (0)

// Min and max longitude:
#define LON_MIN -M_PI
#define LON_MAX  M_PI

// Min and max latitude = atan(sinh(M_PI)):
#define LAT_MIN -1.484422229745332444395
#define LAT_MAX  1.484422229745332444395
