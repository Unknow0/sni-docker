#ifndef __x11_h
#define __x11_h

#include <X11/Xlib.h>
#include <gio/gio.h>

extern Display *display;
extern Window root;

extern gint border;
extern gint icon_size;

Window x11_create();

Pixmap x11_icon_pixmap(gchar *file);

#endif /* __x11_h */
