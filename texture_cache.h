#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "cache.h"

extern uint32_t texture_cache_search (const struct cache_node *in, struct cache_node *out);
extern uint32_t texture_cache_insert (const struct cache_node *loc, const void *rawbits);

extern void texture_cache_destroy (void);
extern bool texture_cache_create  (void);
