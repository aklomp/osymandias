PNG := $(patsubst %.svg,%.png,$(wildcard *.svg))

.PHONY: all clean

all: $(PNG)

%.png: %.svg
	rsvg-convert --format png --output $@ $^
	optipng $@

clean:
	rm $(PNG)
