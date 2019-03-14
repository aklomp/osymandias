#include "../camera.h"
#include "../util.h"
#include "../viewport.h"
#include "../worlds.h"
#include "framerate.h"
#include "signal.h"

static int button_dragged = false;
static int button_pressed = false;
static struct screen_pos button_pressed_pos;
static int button_num;
static int click_halted_autoscroll = 0;

// Translate event coordinates to have origin in bottom left:
// Use a macro and not an inline function because the basic GdkEvent
// does not have x and y members:
#define event_get_pos							\
	struct screen_pos pos;						\
	do {								\
		GtkAllocation allocation;				\
									\
		gtk_widget_get_allocation(widget, &allocation);		\
		pos.x = event->x;					\
		pos.y = allocation.height - event->y;			\
	} while (0)

// Event timestamp in usec:
#define evtime	((int64_t)event->time * 1000)

static gboolean
on_button_press (GtkWidget *widget, GdkEventButton *event)
{
	event_get_pos;

	button_dragged = false;
	button_pressed = true;
	button_pressed_pos = pos;
	button_num = event->button;

	if (button_num == 1)
		viewport_hold_start(&pos);

	// Does the press of this button cause the autoscroll to halt?
	click_halted_autoscroll = world_autoscroll_stop();
	world_autoscroll_measure_down(evtime);

	// Don't propagate further:
	return TRUE;
}

static gboolean
on_button_motion (GtkWidget *widget, GdkEventButton *event)
{
	event_get_pos;

	int dx = pos.x - button_pressed_pos.x;
	int dy = pos.y - button_pressed_pos.y;

	button_dragged = true;

	// Left mouse button:
	if (button_num == 1) {
		world_autoscroll_measure_hold(evtime);
		viewport_hold_move(&pos);
	}
	// Right mouse button:
	if (button_num == 3) {

		if (dx != 0)
			camera_rotate(dx * -0.005f);

		if (dy != 0)
			camera_tilt(dy * 0.005f);
	}
	framerate_repaint();
	button_pressed_pos = pos;

	// Don't propagate further:
	return TRUE;
}

static gboolean
on_button_release (GtkWidget *widget, GdkEventButton *event)
{
	if (!button_pressed)
		return TRUE;

	button_pressed = false;

	// We have just released a drag; kickoff the autoscroller:
	if (button_dragged) {
		world_autoscroll_measure_free(evtime);
		return TRUE;
	}

	// We have a click, not a drag:
	if (click_halted_autoscroll)
		return TRUE;

	// Only recenter the viewport if this is the kind
	// of button press that did not halt the autoscroll:
	event_get_pos;
	viewport_center_at(&pos);
	framerate_repaint();

	// Don't propagate further:
	return TRUE;
}

static gboolean
on_scroll (GtkWidget* widget, GdkEventScroll *event)
{
	switch (event->direction)
	{
	case GDK_SCROLL_UP: {
		event_get_pos;
		viewport_zoom_in(&pos);
		framerate_repaint();
		break;
	}
	case GDK_SCROLL_DOWN: {
		event_get_pos;
		viewport_zoom_out(&pos);
		framerate_repaint();
		break;
	}
	default:
		break;
	}

	// Don't propagate further:
	return TRUE;
}

// Add mouse signal handlers to given widget (a GL area).
void
mouse_signal_connect (GtkWidget *widget)
{
	struct signal map[] = {
		{ "button-press-event",   G_CALLBACK(on_button_press),   GDK_BUTTON_PRESS_MASK   },
		{ "button-release-event", G_CALLBACK(on_button_release), GDK_BUTTON_RELEASE_MASK },
		{ "motion-notify-event",  G_CALLBACK(on_button_motion),  GDK_BUTTON_MOTION_MASK  },
		{ "scroll-event",         G_CALLBACK(on_scroll),         GDK_SCROLL_MASK         },
	};

	signal_connect(widget, map, NELEM(map));
}
