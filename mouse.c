#include <stdbool.h>
#include <gtk/gtk.h>

#include "vector.h"
#include "camera.h"
#include "viewport.h"
#include "worlds.h"

static int button_dragged = false;
static int button_pressed = false;
static int button_pressed_x;
static int button_pressed_y;
static int button_num;
static int click_halted_autoscroll = 0;

// Translate event coordinates to have origin in bottom left:
// Use a macro and not an inline function because the basic GdkEvent
// does not have x and y members:
#define event_get_xy							\
	int x, y;							\
	do {								\
		GtkAllocation allocation;				\
									\
		gtk_widget_get_allocation(widget, &allocation);		\
		x = event->x;						\
		y = allocation.height - event->y;			\
	} while (0)

void
on_button_press (GtkWidget *widget, GdkEventButton *event)
{
	event_get_xy;

	button_dragged = false;
	button_pressed = true;
	button_pressed_x = x;
	button_pressed_y = y;
	button_num = event->button;

	if (button_num == 1)
		viewport_hold_start(x, y);

	// Does the press of this button cause the autoscroll to halt?
	click_halted_autoscroll = world_autoscroll_stop();
	world_autoscroll_measure_down();
}

void
on_button_motion (GtkWidget *widget, GdkEventButton *event)
{
	event_get_xy;

	int dx = x - button_pressed_x;
	int dy = y - button_pressed_y;

	button_dragged = true;

	// Left mouse button:
	if (button_num == 1) {
		world_autoscroll_measure_hold();
		viewport_hold_move(x, y);
	}
	// Right mouse button:
	if (button_num == 3) {

		if (dx != 0)
			camera_rotate(dx * -0.005f);

		if (dy != 0)
			camera_tilt(dy * 0.005f);
	}
	gtk_widget_queue_draw(widget);
	button_pressed_x = x;
	button_pressed_y = y;
}

void
on_button_release (GtkWidget *widget, GdkEventButton *event)
{
	if (!button_pressed)
		return;

	button_pressed = false;

	// We have just released a drag; kickoff the autoscroller:
	if (button_dragged) {
		world_autoscroll_measure_free();
		return;
	}

	// We have a click, not a drag:
	if (click_halted_autoscroll)
		return;

	// Only recenter the viewport if this is the kind
	// of button press that did not halt the autoscroll:
	event_get_xy;
	viewport_center_at(x, y);
	gtk_widget_queue_draw(widget);
}

void
on_mouse_scroll (GtkWidget* widget, GdkEventScroll *event)
{
	switch (event->direction)
	{
	case GDK_SCROLL_UP: {
		event_get_xy;
		viewport_zoom_in(x, y);
		gtk_widget_queue_draw(widget);
		break;
	}
	case GDK_SCROLL_DOWN: {
		event_get_xy;
		viewport_zoom_out(x, y);
		gtk_widget_queue_draw(widget);
		break;
	}
	default:
		break;
	}
}
