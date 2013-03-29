#ifndef TILEPICKER_H
#define TILEPICKER_H

void tilepicker_recalc (void);
bool tilepicker_first (int *x, int *y, int *wd, int *ht, int *zoom);
bool tilepicker_next (int *x, int *y, int *wd, int *ht, int *zoom);
void tilepicker_destroy (void);

#endif	/* TILEPICKER_H */
