#pragma once

#include <stddef.h>

#include <gtk/gtk.h>

// Hold init data for GTK signals:
struct signal {
	const gchar	*signal;
	GCallback	 handler;
	GdkEventMask	 mask;
};

extern void signal_connect (GtkWidget *widget, struct signal *signals, size_t members);
