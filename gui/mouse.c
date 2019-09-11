#include "../camera.h"
#include "../pan.h"
#include "../util.h"
#include "../viewport.h"
#include "../zoom.h"
#include "framerate.h"
#include "signal.h"

#define BUTTON_LEFT	1
#define BUTTON_CENTER	2
#define BUTTON_RIGHT	3

static int button_pressed = false;
static struct viewport_pos button_pressed_pos;
static int button_num;

// Translate event coordinates to have origin in bottom left:
// Use a macro and not an inline function because the basic GdkEvent
// does not have x and y members:
#define event_get_pos							\
	struct viewport_pos pos;					\
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

	button_pressed = true;
	button_pressed_pos = pos;
	button_num = event->button;

	if (button_num == BUTTON_LEFT)
		pan_on_button_down(&pos, evtime);

	// Don't propagate further:
	return TRUE;
}

static gboolean
on_button_motion (GtkWidget *widget, GdkEventButton *event)
{
	event_get_pos;

	if (button_num == BUTTON_LEFT)
		if (pan_on_button_move(&pos, evtime))
			framerate_repaint();

	if (button_num == BUTTON_RIGHT) {
		const int dx = pos.x - button_pressed_pos.x;
		const int dy = pos.y - button_pressed_pos.y;

		if (dx != 0)
			camera_set_rotate(dx * -0.005);

		if (dy != 0)
			camera_set_tilt(dy * 0.005);

		framerate_repaint();
	}

	if (button_num == BUTTON_CENTER) {
		const int dy = pos.y - button_pressed_pos.y;

		if (dy != 0) {
			camera_set_view_angle(camera_get()->view_angle + dy * -0.005);
			framerate_repaint();
		}
	}

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
	event_get_pos;

	if (event->button == BUTTON_LEFT)
		pan_on_button_up(&pos, evtime);

	// Don't propagate further:
	return TRUE;
}

static gboolean
on_scroll (GtkWidget* widget, GdkEventScroll *event)
{
	(void) widget;

	if (event->direction == GDK_SCROLL_UP)
		zoom_in(evtime);

	if (event->direction == GDK_SCROLL_DOWN)
		zoom_out(evtime);

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
