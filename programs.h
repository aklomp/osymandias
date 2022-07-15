#pragma once

#include <stdbool.h>

// Forward declaration to avoid an include loop.
struct program;

extern bool programs_init    (void);
extern void programs_destroy (void);
extern void programs_link    (struct program *program);
