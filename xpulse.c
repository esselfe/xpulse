#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

const char *xwin_version_string = "0.3";
#define OPTION_NONE       0
#define OPTION_VERBOSE    1
#define _NET_WM_STATE_REMOVE        0    // remove/unset property
#define _NET_WM_STATE_ADD           1    // add/set property
#define _NET_WM_STATE_TOGGLE        2    // toggle property
//unsigned int options = OPTION_VERBOSE;
unsigned int options = OPTION_NONE;
Display *display;
Screen *screen;
int screen_num;
int depth;
XSetWindowAttributes wattr;
Window window, root_window;
XEvent ev;
XSizeHints wmsize;
XWMHints wmhint;
XTextProperty wname, iname;
char *window_name = "xpulse 0.2", *icon_name = "xpulse";
unsigned int loopend = 0;
unsigned int sleep_time = 10000; // 1000000 == 1sec

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

int main(int argc, char **argv) {
	nice(10);
	XSetErrorHandler(ErrorFunc);

	signal(SIGUSR1, SignalUSR1);

	display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, "Cannot open a X display!\n");
		return 1;
	}

	screen = XDefaultScreenOfDisplay(display);
	screen_num = XScreenNumberOfScreen(screen);
	depth = XDefaultDepthOfScreen(screen);
	if (options & OPTION_VERBOSE) {
		printf("screen number: %d\n", screen_num);
		printf("screen depth: %d\n", depth);
	}
	root_window = XDefaultRootWindow(display);

	//wattr.background_pixel = BlackPixel(display, screen_num);
	wattr.background_pixel = 0x081018;
	wattr.event_mask = KeyPressMask | ButtonPressMask;
	wattr.cursor = None;
	window = XCreateWindow(display, root_window,
		1280-(1000000/sleep_time+16), 0, 1000000/sleep_time+16, 20, 1, depth, InputOutput, DefaultVisual(display, screen_num),
		CWBackPixel | CWCursor | CWEventMask, &wattr);
	
	wmsize.flags = USPosition | USSize;
	XSetWMNormalHints(display, window, &wmsize);

	wmhint.initial_state = NormalState;
	wmhint.flags = StateHint;
	XSetWMHints(display, window, &wmhint);
	
	XStringListToTextProperty(&window_name, 1, &wname);
	XSetWMName(display, window, &wname);
	XStringListToTextProperty(&icon_name, 1, &iname);
	XSetWMIconName(display, window, &iname);

	Atom window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
	long value = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
	XChangeProperty(display, window, window_type,
		XA_ATOM, 32, PropModeReplace, (unsigned char *)&value, 1);

	XMapWindow(display, window);

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
	gcv.foreground = 0x304050;
	gcv.background = 0x0408c0;
	gcv.line_width = 10;
	GC gc = XCreateGC(display, window, GCForeground|GCBackground|GCLineWidth, &gcv);

	time_t tc, tp = time(NULL);
	int cnt = 0;
	while (!loopend) {
		if (XPending(display)) {
			XNextEvent(display, &ev);
			switch (ev.type) {
			case ButtonPress:
				XBell(display, 80);
				XClearArea(display, window, 0, 3, 800, 12, True);
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
			XClearArea(display, window, 8, 5, 400, 9, True);
		}
		XDrawLine(display, window, gc, 8, 8, 8+(cnt++), 8);
		usleep(sleep_time);
	}

	XUnmapWindow(display, window);
	XDestroyWindow(display, window);
	XCloseDisplay(display);
	return 0;
}

