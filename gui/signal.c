#include <gtk/gtk.h>

#include "signal.h"

void
signal_connect (GtkWidget *widget, struct signal *signals, size_t members)
{
	for (size_t i = 0; i < members; i++) {
		gtk_widget_add_events(widget, signals[i].mask);
		g_signal_connect(widget, signals[i].signal, signals[i].handler, NULL);
	}
}
