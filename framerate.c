#include <stdbool.h>
#include <gtk/gtk.h>

#include "autoscroll.h"

static GtkWidget *canvas = NULL;
static volatile bool refresh_requested;
static void (*paint)(GtkWidget *widget);

static gboolean
framerate_tick (void *data)
{
	(void)(data);	// Unused parameter

	if (!(autoscroll_is_on() || refresh_requested)) {
		return TRUE;
	}
	if (canvas == NULL || !GTK_IS_WIDGET(canvas)) {
		if (autoscroll_is_on()) {
			autoscroll_stop();
		}
		return FALSE;
	}
	refresh_requested = false;
	paint(canvas);

	return TRUE;
}

void
framerate_init (GtkWidget *widget, void (*paint_callback)(GtkWidget *widget))
{
	canvas = widget;

	// This is a pointer to what is usually the on_expose_event handler,
	// i.e. the routine that paints the viewport:
	paint = paint_callback;

	// This is our heartbeat timer:
	g_timeout_add(40, (GSourceFunc)framerate_tick, NULL);
}

void
framerate_destroy (void)
{
	// This causes the heartbeat function to exit:
	canvas = NULL;
}

void
framerate_request_refresh (void)
{
	// This function can be called asynchronously from various threads.
	// This addition is not guaranteed to be atomic, but the worst that
	// can happen is that we miss a frame, or render a spurious one.
	refresh_requested = true;
}
