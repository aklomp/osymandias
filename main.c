#include <stdbool.h>

#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>

#include "mouse.h"
#include "bitmap_mgr.h"
#include "framerate.h"
#include "autoscroll.h"
#include "viewport.h"
#include "layers.h"
#include "layer_background.h"
#include "layer_cursor.h"
#include "layer_blanktile.h"
#include "layer_osm.h"

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
	GdkGLContext  *glcontext;
	GdkGLDrawable *gldrawable;

	glcontext = gtk_widget_get_gl_context(widget);
	gldrawable = gtk_widget_get_gl_drawable(widget);

	gdk_gl_drawable_gl_begin(gldrawable, glcontext);

	viewport_reshape(widget->allocation.width, widget->allocation.height);

	if (gdk_gl_drawable_is_double_buffered(gldrawable)) {
		gdk_gl_drawable_swap_buffers(gldrawable);
	}
	else {
		glFlush();
	}
	gdk_gl_drawable_gl_end(gldrawable);
}

int
main (int argc, char **argv)
{
	int ret = 1;

	int config_attributes[] = {
		GDK_GL_DOUBLEBUFFER,
		GDK_GL_RGBA,
		GDK_GL_RED_SIZE,	1,
		GDK_GL_GREEN_SIZE,	1,
		GDK_GL_BLUE_SIZE,	1,
		GDK_GL_DEPTH_SIZE,	12,
		GDK_GL_ATTRIB_LIST_NONE
	};

	gtk_init(&argc, &argv);

	if (!gdkgl_check(argc, argv)) {
		goto err_0;
	}
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget *canvas = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(window), canvas);

	gtk_widget_add_events(canvas, GDK_BUTTON_PRESS_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON3_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);
	g_signal_connect(canvas, "scroll_event", G_CALLBACK(on_mouse_scroll), NULL);
	g_signal_connect(canvas, "button_press_event", G_CALLBACK(on_button_press), NULL);
	g_signal_connect(canvas, "motion_notify_event", G_CALLBACK(on_button_motion), NULL);
	g_signal_connect(canvas, "button_release_event", G_CALLBACK(on_button_release), NULL);
	g_signal_connect(canvas, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	// Expose events are scheduled through the framerate manager, not handled directly:
	g_signal_connect(canvas, "expose-event", G_CALLBACK(framerate_request_refresh), NULL);

	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 600, 600);

	gtk_widget_set_app_paintable(canvas, TRUE);
	gtk_widget_set_double_buffered(canvas, FALSE);
	gtk_widget_set_double_buffered(window, FALSE);

	GdkGLConfig *glconfig = gdk_gl_config_new(config_attributes);
	gtk_widget_set_gl_capability(canvas, glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE);

	gtk_widget_show_all(window);

	viewport_init();

	layer_background_create();
	layer_cursor_create();
	layer_blanktile_create();
	layer_osm_create();

	framerate_init(canvas, paint_canvas);

	gtk_main();

	framerate_destroy();
	layers_destroy();
	viewport_destroy();

	ret = 0;

err_0:	return ret;
}
