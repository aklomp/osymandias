#include <gtk/gtk.h>

#include "../util.h"
#include "signal.h"

void
signal_connect (GtkWidget *widget, struct signal *signals, size_t members)
{
	FOREACH_NELEM (signals, members, s) {
		gtk_widget_add_events(widget, s->mask);
		g_signal_connect(widget, s->signal, s->handler, NULL);
	}
}
