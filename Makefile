
CFLAGS = -std=c11 -Wall -Werror -O2 -g -D_GNU_SOURCE
LDFLAGS = -lX11 -lXext
PROGNAME = xpulse

.PHONY: default all clean

default: all

all: $(PROGNAME)
	@ls -li --color=auto $(PROGNAME) 2>/dev/null || true

$(PROGNAME): xpulse.c
	gcc $(CFLAGS) $(LDFLAGS) xpulse.c -o $(PROGNAME)

clean:
	@rm -v $(PROGNAME) 2>/dev/null || true

