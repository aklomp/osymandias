#ifndef MOUSE_H
#define MOUSE_H

void on_button_press(GtkWidget*, GdkEventButton*);
void on_button_motion(GtkWidget*, GdkEventButton*);
void on_button_release(GtkWidget*, GdkEventButton*);
void on_mouse_scroll(GtkWidget*, GdkEventScroll*);

#endif
