#ifndef AUTOSCROLL_H
#define AUTOSCROLL_H

void autoscroll_measure_down (void);
void autoscroll_measure_hold (void);
void autoscroll_measure_free (void);
bool autoscroll_stop (void);
bool autoscroll_is_on (void);
bool autoscroll_update (double *restrict x, double *restrict y);
void autoscroll_zoom_in (void);
void autoscroll_zoom_out (void);

#endif /* AUTOSCROLL_H */
