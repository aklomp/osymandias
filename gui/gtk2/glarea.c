#include <stdbool.h>

#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>

#include "../../util.h"
#include "../../viewport.h"
#include "../framerate.h"
#include "../signal.h"
#include "../mouse.h"

static bool is_realized = false;

static bool
opengl_init (int *argc, char ***argv)
{
	int major, minor;

	if (gdk_gl_init_check(argc, argv) == FALSE) {
		fprintf(stderr, "No OpenGL library support.\n");
		return false;
	}
	if (gdk_gl_query_extension() == FALSE) {
		fprintf(stderr, "No OpenGL windowing support.\n");
		return false;
	}
	if (gdk_gl_query_version(&major, &minor) == FALSE) {
		fprintf(stderr, "Could not query OpenGL version.\n");
		return false;
	}
	fprintf(stderr, "OpenGL %d.%d\n", major, minor);
	return true;
}

static void
framerate_start (GtkWidget *canvas)
{
	// Refresh period at 60 Hz = 1000 / 60 msec = 16.666... msec
	guint msec = 17;

	// Kickoff the heartbeat timer:
	g_timeout_add(msec, (GSourceFunc)framerate_on_tick, canvas);
}

static void
on_realize (GtkWidget *widget)
{
	GdkGLContext  *glcontext  = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);
	GtkAllocation  allocation;

	// Initialize viewport inside GL context after realizing GL area:
	gdk_gl_drawable_gl_begin(gldrawable, glcontext);
	gtk_widget_get_allocation(widget, &allocation);
	viewport_init(allocation.width, allocation.height);
	gdk_gl_drawable_gl_end(gldrawable);

	is_realized = true;
}

static void
on_unrealize (GtkWidget *widget)
{
	GdkGLContext  *glcontext  = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);

	// Destroy viewport on unrealizing, but inside GL context:
	gdk_gl_drawable_gl_begin(gldrawable, glcontext);
	viewport_destroy();
	gdk_gl_drawable_gl_end(gldrawable);
}

// Unconditionally redraw:
static gboolean
on_expose (GtkWidget *widget)
{
	GdkGLContext  *glcontext  = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);

	gdk_gl_drawable_gl_begin(gldrawable, glcontext);

	viewport_paint();

	if (gdk_gl_drawable_is_double_buffered(gldrawable)) {
		gdk_gl_drawable_swap_buffers(gldrawable);
	}
	else {
		glFlush();
	}
	gdk_gl_drawable_gl_end(gldrawable);

	// Don't propagate further:
	return TRUE;
}

static gboolean
on_configure (GtkWidget *widget, GdkEventConfigure *event)
{
	// This function is called once before the widget is realized:
	if (!is_realized)
		return FALSE;

	GdkGLContext  *glcontext  = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);

	// Resize viewport within GL context:
	gdk_gl_drawable_gl_begin(gldrawable, glcontext);
	viewport_resize(event->width, event->height);
	gdk_gl_drawable_gl_end(gldrawable);

	return FALSE;
}

static void
canvas_signal_connect (GtkWidget *canvas)
{
	struct signal map[] = {
		{ "expose-event",	G_CALLBACK(on_expose),		0 },
		{ "configure-event",	G_CALLBACK(on_configure),	0 },
		{ "realize",		G_CALLBACK(on_realize),		0 },
		{ "unrealize",		G_CALLBACK(on_unrealize),	0 },
	};

	// Connect native signals:
	signal_connect(canvas, map, NELEM(map));

	// Connect generic mouse-handling signals:
	mouse_signal_connect(canvas);
}

// Populate main window with GL area
void
glarea_add (GtkWidget *window)
{
	// Create canvas:
	GtkWidget *canvas = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(window), canvas);

	gtk_widget_set_app_paintable(canvas, TRUE);
	gtk_widget_set_double_buffered(canvas, FALSE);
	gtk_widget_set_double_buffered(window, FALSE);

	GdkGLConfig *glconfig = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGBA | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE);
	gtk_widget_set_gl_capability(canvas, glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE);

	// Connect signals:
	canvas_signal_connect(canvas);

	// Start frame clock:
	framerate_start(canvas);
}

// Initialize GUI
bool
glarea_init (int *argc, char ***argv)
{
	return opengl_init(argc, argv);
}
