#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "local.h"

extern void autoscroll_measure_down (const struct world_state *, int64_t usec);
extern void autoscroll_measure_hold (const struct world_state *, int64_t usec);
extern void autoscroll_measure_free (const struct world_state *, int64_t usec);
extern bool autoscroll_stop (struct world_state *);
