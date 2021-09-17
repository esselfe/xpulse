#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <getopt.h>
#include <sys/mman.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

const char *xpulse_version_string = "0.4.3";
#define OPTION_NONE       0
#define OPTION_VERBOSE    1
#define _NET_WM_STATE_REMOVE        0    // remove/unset property
#define _NET_WM_STATE_ADD           1    // add/set property
#define _NET_WM_STATE_TOGGLE        2    // toggle property
static const struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'V'},
	{"background", required_argument, NULL, 'b'},
	{"color", required_argument, NULL, 'c'},
	{"height", required_argument, NULL, 'H'},
	{"width", required_argument, NULL, 'W'},
	{"position-x", required_argument, NULL, 'X'},
	{"position-y", required_argument, NULL, 'Y'},
	{NULL, 0, NULL, 0}
};
static const char *short_options = "hVb:c:H:W:X:Y:";
unsigned int sleep_time = 10000; // 1000000 == 1sec, 1000 == 1ms
Display *display;
Screen *screen;
int screen_num;
int depth;
XSetWindowAttributes wattr;
unsigned int winW = 116, winH = 4;
unsigned int winX, winY;
Window window, root_window;
unsigned long background_color = 0x0408c0, foreground_color = 0x304050;
XEvent ev;
XSizeHints wmsize;
XWMHints wmhint;
unsigned int loopend = 0;

int ErrorFunc(Display *display, XErrorEvent *error) {
	printf("Error code %d: ", error->error_code);
	switch(error->error_code) {
	case BadRequest:    // 1
		printf("bad request code\n");
	case BadValue:      // 2
		printf("int parameter out of range\n");
	case BadWindow:     // 3
		printf("parameter not a Window\n");
	case BadPixmap:     // 4
		printf("parameter not a Pixmap\n");
	case BadAtom:       // 5
		printf("parameter not an Atom\n");
	case BadCursor:     // 6
		printf("parameter not a Cursor\n");
	case BadFont:       // 7
		printf("parameter not a Font\n");
	case BadMatch:      // 8
		printf("parameter mismatch\n");
	case BadDrawable:   // 9
		printf("parameter not a Pixmap or Window\n");
	case BadAccess:     // 10
		printf("depending on context:\n");
	case BadAlloc:      // 11
		printf("insufficient resources\n");
	case BadColor:      // 12
		printf("no such colormap\n");
	case BadGC:         // 13
		printf("parameter not a GC\n");
	case BadIDChoice:   // 14
		printf("choice not in range or already used\n");
	case BadName:       // 15
		printf("font or color name doesn't exist\n");
	case BadLength:     // 16
		printf("Request length incorrect\n");
	case BadImplementation: // 17
		printf("server is defective\n");
	}
	return 0;
}

void SignalUSR1(int signum) {

}

void ShowHelp(void) {
	printf("Usage: xpulse { --position-x/-x NUMBER | --position-y/-y NUMBER | --help/-h | --version/-V }\n");
}

void ShowVersion(void) {
	printf("xpulse %s\n", xpulse_version_string);
}

int hex2int(char hex) {
    switch (hex) {
    case '0':
        return 0;
    case '1':
        return 1;
    case '2':
        return 2;
    case '3':
        return 3;
    case '4':
        return 4;
    case '5':
        return 5;
    case '6':
        return 6;
    case '7':
        return 7;
    case '8':
        return 8;
    case '9':
        return 9;
    case 'A':
    case 'a':
        return 10;
    case 'B':
    case 'b':
        return 11;
    case 'C':
    case 'c':
        return 12;
    case 'D':
    case 'd':
		return 13;
    case 'E':
    case 'e':
        return 14;
    case 'F':
    case 'f':
        return 15;
    default:
        return 0;
    }
    return 0;
}

unsigned long hex2ulong(char *hexstr) {
	int cnt;
    unsigned long val = 0;
    for (cnt = 5; cnt >= 0; cnt--) {
        if (cnt == 5)
            val += hex2int(hexstr[cnt]);
        else if (cnt == 4)
            val += hex2int(hexstr[cnt])*16;
        else if (cnt == 3)
            val += hex2int(hexstr[cnt])*16*16;
        else if (cnt == 2)
            val += hex2int(hexstr[cnt])*16*16*16;
        else if (cnt == 1)
            val += hex2int(hexstr[cnt])*16*16*16*16;
        else if (cnt == 0)
            val += hex2int(hexstr[cnt])*16*16*16*16*16;
    }

    return val;
}

int main(int argc, char **argv) {
	nice(10);
	XSetErrorHandler(ErrorFunc);

	signal(SIGUSR1, SignalUSR1);

	int c = 0;
	while (c != -1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c == -1)
			break;
		switch (c) {
		case 'h':
			ShowHelp();
			exit(0);
		case 'V':
			ShowVersion();
			exit(0);
		case 'b':
			if (optarg != NULL)
				background_color = hex2ulong(optarg);
			printf("background: %lu\n", background_color);
			break;
		case 'c':
			if (optarg != NULL)
				foreground_color = hex2ulong(optarg);
			printf("foreground: %lu\n", foreground_color);
			break;
		case 'H':
			if (optarg != NULL)
				winH = atoi(optarg);
			break;
		case 'W':
			if (optarg != NULL)
				winW = atoi(optarg);
			break;
		case 'X':
			if (optarg != NULL)
				winX = atoi(optarg);
			break;
		case 'Y':
			if (optarg != NULL)
				winY = atoi(optarg);
			break;
		}
	}

	display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, "Cannot open a X display!\n");
		return 1;
	}

	screen = XDefaultScreenOfDisplay(display);
	screen_num = XScreenNumberOfScreen(screen);
	depth = XDefaultDepthOfScreen(screen);
	root_window = XDefaultRootWindow(display);

	//wattr.background_pixel = BlackPixel(display, screen_num);
	wattr.background_pixel = 0x081018;
	wattr.event_mask = KeyPressMask | ButtonPressMask;
	wattr.cursor = None;
	window = XCreateWindow(display, root_window,
		//winX, winY, 1000000/sleep_time+16, 4, 1, depth, InputOutput, DefaultVisual(display, screen_num),
		winX, winY, winW, winH, 1, depth, InputOutput, DefaultVisual(display, screen_num),
		CWBackPixel | CWCursor | CWEventMask, &wattr);
	
	wmsize.flags = USPosition | USSize;
	XSetWMNormalHints(display, window, &wmsize);

	wmhint.initial_state = NormalState;
	wmhint.flags = StateHint;
	XSetWMHints(display, window, &wmhint);
	
	Atom window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
	long value = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
	XChangeProperty(display, window, window_type,
		XA_ATOM, 32, PropModeReplace, (unsigned char *)&value, 1);

	XMapWindow(display, window);

	XWindowChanges ch;
	ch.x = winX;
	ch.y = winY;
	XConfigureWindow(display, window, CWX | CWY, &ch);

	Atom wmStateAbove = XInternAtom(display, "_NET_WM_STATE_ABOVE", 1);
	Atom wmStateSticky = XInternAtom(display, "_NET_WM_STATE_STICKY", 1);
	Atom wmNetWmState = XInternAtom(display, "_NET_WM_STATE", 1);
	if( wmStateAbove != None ) {
		XClientMessageEvent xclient;
		memset( &xclient, 0, sizeof (xclient) );
    //
    //window  = the respective client window
    //message_type = _NET_WM_STATE
    //format = 32
    //data.l[0] = the action, as listed below
    //data.l[1] = first property to alter
    //data.l[2] = second property to alter
    //data.l[3] = source indication (0-unk,1-normal app,2-pager)
    //other data.l[] elements = 0
    //
		xclient.type = ClientMessage;
		xclient.window = window;
		xclient.message_type = wmNetWmState;
		xclient.format = 32;
		xclient.data.l[0] = _NET_WM_STATE_ADD; // add ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
		xclient.data.l[1] = wmStateAbove;
		xclient.data.l[2] = wmStateSticky;
		xclient.data.l[3] = 0;
		xclient.data.l[4] = 0;
		XSendEvent( display,
			// window - wrong, not app window, send to root window!
			root_window, // !! DefaultRootWindow( display ) !!!
			False,
			SubstructureRedirectMask | SubstructureNotifyMask,
			(XEvent *)&xclient );
	}

	XGCValues gcv;
	gcv.foreground = foreground_color;
	gcv.background = background_color;
	gcv.line_width = winH;
	GC gc = XCreateGC(display, window, GCForeground|GCBackground|GCLineWidth, &gcv);

	time_t tc, tp = time(NULL);
	int cnt = 0;
	while (!loopend) {
		if (XPending(display)) {
			XNextEvent(display, &ev);
			switch (ev.type) {
			case ButtonPress:
				XBell(display, 80);
				XClearArea(display, window, 0, 0, winW, winH, True);
				break;
			case KeyPress:
				if (ev.xkey.keycode == 9)
					loopend = 1;
				break;
			}
		}
		tc = time(NULL);
		if (tc > tp) {
			tp = tc;
			cnt = 0;
			XClearArea(display, window, 0, 0, winW, winH, True);
		}
		XDrawLine(display, window, gc, 0, winH/2, cnt++, winH/2);
		usleep(sleep_time);
	}

	XUnmapWindow(display, window);
	XDestroyWindow(display, window);
	XCloseDisplay(display);
	return 0;
}

