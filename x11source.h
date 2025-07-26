#ifndef __x11source_h
#define __x11source_h

#include <glib.h>
#include <X11/Xlib.h>

typedef struct {
	GSource source;
	Display *dpy;
} X11Source;

GSource *x11source_new(Display *dpy);

#endif /* __x11source_h */
