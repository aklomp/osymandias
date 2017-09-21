CFLAGS	+= -std=c11 -D_GNU_SOURCE -O3 -msse3 -ffast-math
CFLAGS	+= -Wall -Wextra -pedantic -g

LDFLAGS	+= -lm -lpng -lpthread

# Compile for GTK+-3 if possible, else fall back to GTK+-2.
# Need at least GTK+-3.16 for native GtkGLArea:
GTK3 := $(shell pkg-config --atleast-version=3.16 gtk+-3.0 && echo 1 || echo 0)

ifeq ($(GTK3),1)
  GTK_CFLAGS  := $(shell pkg-config --cflags gtk+-3.0)
  GTK_LDFLAGS := $(shell pkg-config --libs   gtk+-3.0)

  GTKGL_CFLAGS  := -DGL_GLEXT_PROTOTYPES
  GTKGL_LDFLAGS := -lGL -lGLU

  OBJS_GTK_GTKGL = gui/gtk3/glarea.o
else
  GTK_CFLAGS  := $(shell pkg-config --cflags gtk+-2.0 glib-2.0)
  GTK_LDFLAGS := $(shell pkg-config --libs   gtk+-2.0 glib-2.0)

  GTKGL_CFLAGS  := $(shell pkg-config --cflags gtkglext-1.0 gdkglext-1.0) -DGL_GLEXT_PROTOTYPES
  GTKGL_LDFLAGS := $(shell pkg-config --libs   gtkglext-1.0 gdkglext-1.0)

  OBJS_GTK_GTKGL = gui/gtk2/glarea.o
endif

PROG = osymandias

OBJS = \
  png.o \
  main.o \
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
  gui/framerate.o \
  gui/mouse.o \
  gui/signal.o \

OBJS_GTKGL = \
  glutil.o \
  programs.o \
  $(patsubst %.c,%.o,$(wildcard layers/*.c)) \
  $(patsubst %.c,%.o,$(wildcard programs/*.c))

OBJS_GTK_GTKGL += \
  gui.o \
  viewport.o \

OBJS_BIN = \
  $(patsubst %.png,%.o,$(wildcard textures/*.png)) \
  $(patsubst %.glsl,%.o,$(wildcard shaders/*/*.glsl))

all: $(PROG)

$(PROG): $(OBJS) $(OBJS_GTK) $(OBJS_GTKGL) $(OBJS_GTK_GTKGL) $(OBJS_BIN) $(OBJS_BIN)
	$(CC) $(LDFLAGS) $(GTK_LDFLAGS) $(GTKGL_LDFLAGS) -o $@ $^

%.o: %.glsl
	$(LD) --relocatable --format=binary -o $@ $^

%.o: %.png
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
