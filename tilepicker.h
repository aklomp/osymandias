#ifndef TILEPICKER_H
#define TILEPICKER_H

void tilepicker_recalc (void);
bool tilepicker_first (float *x, float *y, float *wd, float *ht, int *zoom, struct vector coords[4], struct vector normal[4]);
bool tilepicker_next (float *x, float *y, float *wd, float *ht, int *zoom, struct vector coords[4], struct vector normal[4]);
void tilepicker_destroy (void);

#endif	/* TILEPICKER_H */
