#ifndef AUTOSCROLL_H
#define AUTOSCROLL_H

void autoscroll_measure_down (const int x, const int y);
void autoscroll_measure_hold (const int x, const int y);
void autoscroll_may_start (const int x, const int y);
bool autoscroll_stop (void);
bool autoscroll_is_on (void);
bool autoscroll_update (int *restrict new_dx, int *restrict new_dy);

#endif /* AUTOSCROLL_H */
