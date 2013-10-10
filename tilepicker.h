#ifndef TILEPICKER_H
#define TILEPICKER_H

void tilepicker_recalc (void);
bool tilepicker_first (int *x, int *y, int *wd, int *ht, int *zoom, float p[4][3]);
bool tilepicker_next (int *x, int *y, int *wd, int *ht, int *zoom, float p[4][3]);
void tilepicker_destroy (void);

#endif	/* TILEPICKER_H */
