CFLAGS	+= -std=c11 -D_GNU_SOURCE
CFLAGS	+= -march=native -O3 -ffast-math
CFLAGS	+= -Wall -Wextra -pedantic -g
CFLAGS	+= -I lib/vec/include

LDLIBS  += -lpng -pthread -lm

# Configure verbosity:
VERBOSE ?= no
ifeq ($(VERBOSE),no)
  ECHO = @echo
  MAKEFLAGS += -s
else
  ECHO = @true
endif

GTK_CFLAGS := $(shell pkg-config --cflags gtk+-3.0)
GTK_LDLIBS := $(shell pkg-config --libs   gtk+-3.0)

GTKGL_CFLAGS := -DGL_GLEXT_PROTOTYPES
GTKGL_LDLIBS := -lGL -lGLU

PROG = osymandias
SRCS = $(wildcard *.c) \
       $(wildcard */*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

OBJS_BIN = \
  $(patsubst %.png,%.o,$(wildcard textures/*.png)) \
  $(patsubst %.glsl,%.o,$(wildcard shaders/*/*.glsl))

.PHONY: clean

$(PROG): $(OBJS) $(OBJS_BIN)
	$(ECHO) '  LD    $@'
	$(CC) $(LDFLAGS) -o $@ $^ $(GTKGL_LDLIBS) $(GTK_LDLIBS) $(LDLIBS)

%.o: %.glsl
	$(ECHO) '  BIN   $@'
	$(LD) --relocatable --format=binary -o $@ $^

%.o: %.png
	$(ECHO) '  BIN   $@'
	$(LD) --relocatable --format=binary -o $@ $^

$(OBJS): %.o: %.c
	$(ECHO) '  CC    $@'
	$(CC) $(GTKGL_CFLAGS) $(GTK_CFLAGS) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS_BIN) $(OBJS) $(PROG)
