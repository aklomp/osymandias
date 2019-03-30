#pragma once

#include <stdbool.h>
#include <stdint.h>

struct tilepicker2 {
	uint32_t x;
	uint32_t y;
	uint32_t zoom;
} __attribute__((packed));

extern void tilepicker2_recalc (void);
extern const struct tilepicker2 *tilepicker2_first (void);
extern const struct tilepicker2 *tilepicker2_next  (void);
