#include <stdbool.h>
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

void
on_button_press (GtkWidget *widget, GdkEventButton *event)
{
	button_dragged = false;
	button_pressed = true;
	button_pressed_x = EVENT_X;
	button_pressed_y = EVENT_Y;
}

void
on_button_motion (GtkWidget *widget, GdkEventButton *event)
{
	int dx = EVENT_X - button_pressed_x;
	int dy = EVENT_Y - button_pressed_y;

	button_dragged = true;
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
	if (event->type == GDK_BUTTON_RELEASE && !button_dragged) {
		viewport_center_at(EVENT_X, EVENT_Y);
		gtk_widget_queue_draw(widget);
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
