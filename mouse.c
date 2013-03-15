#include <stdbool.h>
#include <gtk/gtk.h>

#include "viewport.h"
#include "autoscroll.h"

#define EVENT_X		(int)(event->x)
#define EVENT_Y		(int)(widget->allocation.height - (int)event->y)

static int button_dragged = false;
static int button_pressed = false;
static int button_pressed_x;
static int button_pressed_y;
static int button_num;
static int click_halted_autoscroll = 0;

void
on_button_press (GtkWidget *widget, GdkEventButton *event)
{
	button_dragged = false;
	button_pressed = true;
	button_pressed_x = EVENT_X;
	button_pressed_y = EVENT_Y;
	button_num = event->button;

	if (button_num == 1) {
		viewport_hold_start(EVENT_X, EVENT_Y);
	}
	// Does the press of this button cause the autoscroll to halt?
	click_halted_autoscroll = autoscroll_is_on();
	autoscroll_stop();
	autoscroll_measure_down(button_pressed_x, button_pressed_y);
}

void
on_button_motion (GtkWidget *widget, GdkEventButton *event)
{
	int dx = EVENT_X - button_pressed_x;
	int dy = EVENT_Y - button_pressed_y;

	button_dragged = true;

	// Left mouse button:
	if (button_num == 1) {
		autoscroll_measure_hold(EVENT_X, EVENT_Y);
		viewport_hold_move(EVENT_X, EVENT_Y);
	}
	// Right mouse button:
	if (button_num == 3) {
		viewport_rotate(dx);
		viewport_tilt(dy);
	}
	gtk_widget_queue_draw(widget);
	button_pressed_x = EVENT_X;
	button_pressed_y = EVENT_Y;
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
			// We have a click, not a drag:
			if (autoscroll_is_on()) {
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
			// We have just released a drag; kickoff the autoscroller:
			autoscroll_may_start(EVENT_X, EVENT_Y);
			if (autoscroll_is_on()) {
				gtk_widget_queue_draw(widget);
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
