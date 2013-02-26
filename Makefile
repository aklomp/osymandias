CFLAGS += -std=c99 -D_GNU_SOURCE -O3 -Wall -Wextra -pedantic -g
LDFLAGS += -lm -lpng -lpthread -lrt

GTK_CFLAGS = `pkg-config --cflags gtk+-2.0 glib-2.0`
GTK_LDFLAGS = `pkg-config --libs gtk+-2.0 glib-2.0`

GTKGL_CFLAGS = `pkg-config --cflags gtkglext-1.0 gdkglext-1.0` -DGL_GLEXT_PROTOTYPES
GTKGL_LDFLAGS = `pkg-config --libs gtkglext-1.0 gdkglext-1.0`

PROG = osymandias

OBJS = \
  world.o \
  layers.o \
  xylist.o \
  diskcache.o \
  pngloader.o \
  autoscroll.o \
  bitmap_mgr.o \

OBJS_GTK = \
  mouse.o \
  framerate.o \

OBJS_GTKGL = \
  shaders.o \
  layer_cursor.o \
  layer_background.o \

OBJS_GTK_GTKGL = \
  main.o \
  viewport.o \

GLSL = bkgd.glsl.h cursor.glsl.h

all: $(PROG)

$(PROG): $(OBJS) $(OBJS_GTK) $(OBJS_GTKGL) $(OBJS_GTK_GTKGL)
	$(CC) $(LDFLAGS) $(GTK_LDFLAGS) $(GTKGL_LDFLAGS) -o $@ $^

$(OBJS_GTK_GTKGL): %.o: %.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(GTKGL_CFLAGS) -c $< -o $@

$(OBJS_GTKGL): %.o: %.c $(GLSL)
	$(CC) $(CFLAGS) $(GTKGL_CFLAGS) -c $<

$(OBJS_GTK): %.o: %.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.glsl.h: %.glsl
	awk '{ printf("\"%s\"\n", $$0) } END { print ";" }' < $^ > $@

.PHONY: clean all

clean:
	rm -f $(OBJS_GTK_GTKGL) $(OBJS_GTKGL) $(OBJS_GTK) $(OBJS) $(PROG) $(GLSL)
