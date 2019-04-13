#include <stdbool.h>

#include <gtk/gtk.h>
#include <GL/gl.h>

#include "../../util.h"
#include "../../viewport.h"
#include "../framerate.h"
#include "../signal.h"
#include "../mouse.h"

// Mark one or two parameters as being unused:
#define UNUSED(...) ((void) ((void) __VA_ARGS__))

// Start the frame clock.
static void
framerate_start (GtkGLArea *glarea)
{
	// Get frame clock:
	GdkGLContext *glcontext = gtk_gl_area_get_context(glarea);
	GdkWindow *glwindow = gdk_gl_context_get_window(glcontext);
	GdkFrameClock *frame_clock = gdk_window_get_frame_clock(glwindow);

	// Call framerate_tick() for each frame tick:
	g_signal_connect_swapped
		( frame_clock
		, "update"
		, G_CALLBACK(framerate_on_tick)
		, glarea
		) ;

	// Start the frame clock:
	gdk_frame_clock_begin_updating(frame_clock);
}

// Repaint the frame unconditionally.
// Don't call directly, this call is wrapped by GtkGLArea's 'draw' handler.
gboolean
on_render (GtkGLArea *glarea, GdkGLContext *context)
{
	UNUSED(glarea, context);

	// Call the renderer:
	viewport_paint();

	// Don't propagate signal:
	return TRUE;
}

static void
on_realize (GtkGLArea *glarea)
{
	// Make current:
	gtk_gl_area_make_current(glarea);

	// Enable depth buffer:
	gtk_gl_area_set_has_depth_buffer(glarea, TRUE);

	// Initialize viewport inside GL context after realizing GL area:
	GtkAllocation allocation;
	gtk_widget_get_allocation(GTK_WIDGET(glarea), &allocation);
	viewport_init(allocation.width, allocation.height);

	// Start frame clock:
	framerate_start(glarea);
}

static void
on_resize (GtkGLArea *area, gint width, gint height)
{
	UNUSED(area);

	viewport_resize(width, height);
}

static void
glarea_signal_connect (GtkWidget *glarea)
{
	struct signal map[] = {
		{ "realize", G_CALLBACK(on_realize), 0 },
		{ "render",  G_CALLBACK(on_render),  0 },
		{ "resize",  G_CALLBACK(on_resize),  0 },
	};

	// Connect native signals:
	signal_connect(glarea, map, NELEM(map));

	// Connect generic mouse-handling signals:
	mouse_signal_connect(glarea);
}

// Populate main window with GtkGLArea
void
glarea_add (GtkWidget *window)
{
	// Create GtkGLArea, add to window:
	GtkWidget *glarea = gtk_gl_area_new();
	gtk_container_add(GTK_CONTAINER(window), glarea);

	// Connect signals:
	glarea_signal_connect(glarea);
}

// Initialize GUI
bool
glarea_init (int *argc, char ***argv)
{
	UNUSED(argc, argv);

	return true;
}
