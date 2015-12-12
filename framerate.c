#include <stdbool.h>
#include <gtk/gtk.h>

#include "worlds.h"

static struct {
	GtkWidget *canvas;
	bool repaint;
	void (*paint)(GtkWidget *widget);
} state;

static gboolean
timer_tick (void)
{
	// Get current time:
	gint64 now = g_get_monotonic_time();

	// Feed timer tick to worlds, query repaint:
	state.repaint |= world_timer_tick(now);

	// Quit timer loop if canvas no longer exists:
	if (!state.canvas || !GTK_IS_WIDGET(state.canvas))
		return FALSE;

	// Yield if nothing to do:
	if (!state.repaint)
		return TRUE;

	// Else call the paint callback:
	state.paint(state.canvas);
	state.repaint = false;

	return TRUE;
}

void
framerate_init (GtkWidget *canvas, void (*paint)(GtkWidget *))
{
	state.canvas  = canvas;
	state.paint   = paint;
	state.repaint = true;

	// Refresh period at 60 Hz = 1000 / 60 msec = 16.666... msec
	guint msec = 17;

	// Kickoff the heartbeat timer:
	g_timeout_add(msec, (GSourceFunc)timer_tick, NULL);
}

void
framerate_destroy (void)
{
	// This will terminate the timer loop on next tick:
	state.canvas = NULL;
}

void
framerate_repaint (void)
{
	// Request a screen repaint on next timer tick:
	state.repaint = true;
}
