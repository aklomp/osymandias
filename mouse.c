#include <stdbool.h>
#include <time.h>
#include <gtk/gtk.h>

#include "viewport.h"

#define EVENT_X		(int)(event->x)
#define EVENT_Y		(int)(widget->allocation.height - (int)event->y)

static int last_x = 0;
static int last_y = 0;
static bool button_dragged = false;
static bool button_pressed = false;
static int button_pressed_x;
static int button_pressed_y;
static int button_down_x;
static int button_down_y;
static struct timespec button_down_t;
static struct timespec button_motion_t;

static float autoscroll_dx = 0;		// pixels
static float autoscroll_dy = 0;		// pixels
static int autoscroll_dt = 0;		// milliseconds
static bool autoscroll_on = false;
static int click_halted_autoscroll = 0;
static float autoscroll_base_x;
static float autoscroll_base_y;
static struct timespec autoscroll_started;

static gboolean
autoscroll_tick (GtkWidget* widget)
{
	struct timespec now;

	// Process one tick of the autoscroller:

	clock_gettime(CLOCK_REALTIME, &now);
	float dt = ((float)now.tv_sec + ((float)now.tv_nsec) / 1000000000.0)
	         - ((float)autoscroll_started.tv_sec + ((float)autoscroll_started.tv_nsec) / 1000000000.0);

	float dx = autoscroll_dx * dt - autoscroll_base_x;
	float dy = autoscroll_dy * dt - autoscroll_base_y;

	autoscroll_base_x = dt * autoscroll_dx;
	autoscroll_base_y = dt * autoscroll_dy;

	viewport_scroll(dx, dy);
	gtk_widget_queue_draw(widget);
	return autoscroll_on;
}

static void
autoscroll_start (GtkWidget *widget, const int dx, const int dy, const float dt)
{
	// The 2.0 coefficient is "friction":
	autoscroll_dx = (float)dx / 2.0 / dt;
	autoscroll_dy = (float)dy / 2.0 / dt;
	autoscroll_dt = 40.0;

	autoscroll_base_x = 0.0;
	autoscroll_base_y = 0.0;
	clock_gettime(CLOCK_REALTIME, &autoscroll_started);

	if (autoscroll_dt > 0.0 && !(autoscroll_dx == 0.0 && autoscroll_dy == 0.0)) {
		g_timeout_add(autoscroll_dt, (GSourceFunc)autoscroll_tick, (gpointer)widget);
		autoscroll_on = true;
	}
	autoscroll_tick(widget);
}

static void
autoscroll_stop (void)
{
	autoscroll_on = false;
}

void
on_button_press (GtkWidget *widget, GdkEventButton *event)
{
	button_dragged = false;
	button_pressed = true;
	button_pressed_x = button_down_x = EVENT_X;
	button_pressed_y = button_down_y = EVENT_Y;
	clock_gettime(CLOCK_REALTIME, &button_down_t);

	// Does the press of this button cause the autoscroll to halt?
	click_halted_autoscroll = (autoscroll_on == TRUE);
	autoscroll_stop();
}

void
on_button_motion (GtkWidget *widget, GdkEventButton *event)
{
	int dx = EVENT_X - button_pressed_x;
	int dy = EVENT_Y - button_pressed_y;

	button_dragged = true;
	clock_gettime(CLOCK_REALTIME, &button_motion_t);
	viewport_scroll(dx, dy);
	gtk_widget_queue_draw(widget);

	button_pressed_x = EVENT_X;
	button_pressed_y = EVENT_Y;

	return;

	// Only initialize vars if we actually need them:
	{
		int x = EVENT_X;
		int y = EVENT_Y;
		int dx = x - last_x;
		int dy = y - last_y;

		if (!dx && !dy) {
			return;
		}
		viewport_scroll(dx, dy);
		gtk_widget_queue_draw(widget);
		last_x = x;
		last_y = y;
	}
}

void
on_button_release (GtkWidget *widget, GdkEventButton *event)
{
	if (!button_pressed) {
		return;
	}
	button_pressed = false;
	if (event->type == GDK_BUTTON_RELEASE) {
		if (!button_dragged) {
			if (autoscroll_on) {
				autoscroll_stop();
			}
			else if (!click_halted_autoscroll) {
				// Only recenter the viewport if this is the kind
				// of button press that did not halt the autoscroll:
				viewport_center_at(EVENT_X, EVENT_Y);
			}
			gtk_widget_queue_draw(widget);
		}
		else {
			float dt;
			float dtm;
			struct timespec button_release_t;
			int dx = EVENT_X - button_down_x;
			int dy = EVENT_Y - button_down_y;
			int dxm = EVENT_X - button_pressed_x;
			int dym = EVENT_Y - button_pressed_y;

			clock_gettime(CLOCK_REALTIME, &button_release_t);

			// Time since last button down: this is the base to calculate the displacement from:
			dt = ((float)button_release_t.tv_sec + ((float)button_release_t.tv_nsec) / 1000000000.0)
			   - ((float)button_down_t.tv_sec + ((float)button_down_t.tv_nsec) / 1000000000.0);

			// Time since last mouse movement: if the user is not moving the mouse, don't autoscroll:
			dtm = ((float)button_release_t.tv_sec + ((float)button_release_t.tv_nsec) / 1000000000.0)
			   - ((float)button_motion_t.tv_sec + ((float)button_motion_t.tv_nsec) / 1000000000.0);

			// Mouse stationary in single location? No autoscroll:
			if (dtm > 0.1 && dxm > -12 && dxm < 12 && dym > -12 && dym < 12) {
				autoscroll_stop();
			}
			else {
				autoscroll_start(widget, dx, dy, dt);
			}
		}
	}
}

void
on_mouse_scroll (GtkWidget* widget, GdkEventScroll *event)
{
	int click_x;
	int click_y;

	switch (event->direction)
	{
		case GDK_SCROLL_UP:
			click_x = EVENT_X;
			click_y = EVENT_Y;
			viewport_zoom_in(click_x, click_y);
			gtk_widget_queue_draw(widget);
			break;

		case GDK_SCROLL_DOWN:
			click_x = EVENT_X;
			click_y = EVENT_Y;
			viewport_zoom_out(click_x, click_y);
			gtk_widget_queue_draw(widget);
			break;

		default:
			break;
	}
}
