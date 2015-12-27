CFLAGS += -std=c99 -D_GNU_SOURCE -O3 -msse3 -ffast-math -Wall -Wextra -pedantic -g
LDFLAGS += -lm -lpng -lpthread

GTK_CFLAGS  := $(shell pkg-config --cflags gtk+-2.0 glib-2.0)
GTK_LDFLAGS := $(shell pkg-config --libs   gtk+-2.0 glib-2.0)

GTKGL_CFLAGS  := $(shell pkg-config --cflags gtkglext-1.0 gdkglext-1.0) -DGL_GLEXT_PROTOTYPES
GTKGL_LDFLAGS := $(shell pkg-config --libs   gtkglext-1.0 gdkglext-1.0)

PROG = osymandias

OBJS = \
  camera.o \
  matrix.o \
  layers.o \
  worlds.o \
  quadtree.o \
  diskcache.o \
  pngloader.o \
  inlinebin.o \
  bitmap_mgr.o \
  threadpool.o \
  tiledrawer.o \
  tilepicker.o \
  $(patsubst %.c,%.o,$(wildcard worlds/*.c))

OBJS_GTK = \
  mouse.o \
  framerate.o \

OBJS_GTKGL = \
  programs.o \
  $(patsubst %.c,%.o,$(wildcard layers/*.c)) \
  $(patsubst %.c,%.o,$(wildcard programs/*.c))

OBJS_GTK_GTKGL = \
  main.o \
  viewport.o \

OBJS_BIN = \
  $(patsubst %.glsl,%.o,$(wildcard shaders/*/*.glsl))

all: $(PROG)

$(PROG): $(OBJS) $(OBJS_GTK) $(OBJS_GTKGL) $(OBJS_GTK_GTKGL) $(OBJS_BIN)
	$(CC) $(LDFLAGS) $(GTK_LDFLAGS) $(GTKGL_LDFLAGS) -o $@ $^

$(OBJS_BIN): %.o: %.glsl
	$(LD) --relocatable --format=binary -o $@ $^

$(OBJS_GTK_GTKGL): %.o: %.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(GTKGL_CFLAGS) -c $< -o $@

$(OBJS_GTKGL): %.o: %.c
	$(CC) $(CFLAGS) $(GTKGL_CFLAGS) -c $< -o $@

$(OBJS_GTK): %.o: %.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean all

clean:
	rm -f $(OBJS_BIN) $(OBJS_GTK_GTKGL) $(OBJS_GTKGL) $(OBJS_GTK) $(OBJS) $(PROG)
