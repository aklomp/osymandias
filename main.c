#include <stdbool.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>

#include "mouse.h"
#include "framerate.h"
#include "autoscroll.h"
#include "viewport.h"

struct signal {
	const gchar	*signal;
	GCallback	 handler;
	GdkEventMask	 mask;
};

static bool is_realized = false;

static bool
gdkgl_check (int argc, char **argv)
{
	int major, minor;

	if (gdk_gl_init_check(&argc, &argv) == FALSE) {
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
paint_canvas (GtkWidget *widget)
{
	GdkGLContext  *glcontext  = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);

	gdk_gl_drawable_gl_begin(gldrawable, glcontext);

	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	viewport_resize(allocation.width, allocation.height);

	if (gdk_gl_drawable_is_double_buffered(gldrawable)) {
		gdk_gl_drawable_swap_buffers(gldrawable);
	}
	else {
		glFlush();
	}
	gdk_gl_drawable_gl_end(gldrawable);
}

static void
on_key_press (GtkWidget *widget, GdkEventKey *event)
{
	(void)widget;

	switch (event->keyval)
	{
	case GDK_KEY_p:
	case GDK_KEY_P:
		viewport_mode_set(VIEWPORT_MODE_PLANAR);
		framerate_request_refresh();
		break;

	case GDK_KEY_s:
	case GDK_KEY_S:
		viewport_mode_set(VIEWPORT_MODE_SPHERICAL);
		framerate_request_refresh();
		break;
	}
}

static void
on_realize (GtkWidget *widget)
{
	GdkGLContext  *glcontext  = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);

	// Initialize viewport after realizing GL area, inside GL context:
	gdk_gl_drawable_gl_begin(gldrawable, glcontext);
	viewport_init();
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
connect_signals (GtkWidget *widget, struct signal *signals, size_t members)
{
	for (size_t i = 0; i < members; i++) {
		gtk_widget_add_events(widget, signals[i].mask);
		g_signal_connect(widget, signals[i].signal, signals[i].handler, NULL);
	}
}

static void
connect_canvas_signals (GtkWidget *canvas)
{
	struct signal signals[] = {
		{ "button-press-event",		G_CALLBACK(on_button_press),		GDK_BUTTON_PRESS_MASK	},
		{ "button-release-event",	G_CALLBACK(on_button_release),		GDK_BUTTON_RELEASE_MASK	},
		{ "scroll-event",		G_CALLBACK(on_mouse_scroll),		GDK_SCROLL_MASK		},
		{ "motion-notify-event",	G_CALLBACK(on_button_motion),		GDK_BUTTON_MOTION_MASK	},
		{ "expose-event",		G_CALLBACK(framerate_request_refresh),	0			},
		{ "configure-event",		G_CALLBACK(on_configure),		0			},
		{ "realize",			G_CALLBACK(on_realize),			0			},
		{ "unrealize",			G_CALLBACK(on_unrealize),		0			},
	};

	connect_signals(canvas, signals, sizeof(signals) / sizeof(signals[0]));
}

static void
connect_window_signals (GtkWidget *window)
{
	struct signal signals[] = {
		{ "destroy",		G_CALLBACK(gtk_main_quit),	0			},
		{ "key-press-event",	G_CALLBACK(on_key_press),	GDK_KEY_PRESS_MASK	},
	};

	connect_signals(window, signals, sizeof(signals) / sizeof(signals[0]));
}

int
main (int argc, char **argv)
{
	int ret = 1;

	gtk_init(&argc, &argv);

	if (!gdkgl_check(argc, argv)) {
		goto err_0;
	}
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget *canvas = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(window), canvas);

	connect_canvas_signals(canvas);
	connect_window_signals(window);

	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 600, 600);

	gtk_widget_set_app_paintable(canvas, TRUE);
	gtk_widget_set_double_buffered(canvas, FALSE);
	gtk_widget_set_double_buffered(window, FALSE);

	GdkGLConfig *glconfig = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGBA | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE);
	gtk_widget_set_gl_capability(canvas, glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE);

	gtk_widget_show_all(window);

	framerate_init(canvas, paint_canvas);

	gtk_main();

	framerate_destroy();

	ret = 0;

err_0:	return ret;
}
