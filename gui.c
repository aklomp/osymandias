#include <stdbool.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "framerate.h"
#include "viewport.h"
#include "worlds.h"
#include "gui/local.h"
#include "gui/signal.h"

static void
on_key_press (GtkWidget *widget, GdkEventKey *event)
{
	(void)widget;

	switch (event->keyval)
	{
	case GDK_KEY_p:
	case GDK_KEY_P:
		world_set(WORLD_PLANAR);
		framerate_repaint();
		break;

	case GDK_KEY_s:
	case GDK_KEY_S:
		world_set(WORLD_SPHERICAL);
		framerate_repaint();
		break;
	}
}

static void
connect_window_signals (GtkWidget *window)
{
	struct signal map[] = {
		{ "destroy",		G_CALLBACK(gtk_main_quit),	0			},
		{ "key-press-event",	G_CALLBACK(on_key_press),	GDK_KEY_PRESS_MASK	},
	};

	signal_connect(window, map, sizeof(map) / sizeof(map[0]));
}

// Initialize GUI
bool
gui_init (int *argc, char ***argv)
{
	// Initialize GTK:
	if (!gtk_init_check(argc, argv))
		return false;

	// Version-specific initialization:
	if (!glarea_init(argc, argv))
		return false;

	return true;
}

// Run GUI after initializing
bool
gui_run (void)
{
	// This function works in both GTK+-2 and GTK+-3 mode.
	// It calls out to version-specific implementations in gui/ to do
	// custom work.

	// Create window:
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	// Call version-specific function to populate window:
	glarea_add(window);

	// Connect signals to window:
	connect_window_signals(window);

	// Set window position and show:
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 600, 600);
	gtk_widget_show_all(window);

	// Run the GTK event loop:
	gtk_main();

	return true;
}