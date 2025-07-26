#include "utils.h"
#include "x11source.h"
#include "x11.h"
#include "host_item.h"

#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <glib.h>

static void x11_draw_item(Display *dpy, ItemData *item) {
	GC gc = XCreateGC(dpy, item->win, 0, NULL);
	
	if(item->icon_x11 != None)
		XCopyArea(dpy, item->icon_x11, item->win, gc, 0, 0, icon_size, icon_size, 0, 0);
	
	XFreeGC(dpy, gc);
	XFlush(dpy);
}

gboolean x11source_prepare(GSource* source, UNUSED gint* timeout) {
	*timeout=-1;
	return XPending(((X11Source*)source)->dpy)>0;
}

gboolean x11source_check(UNUSED GSource* source) {
	return XPending(((X11Source*)source)->dpy)>0;
}

gboolean x11source_dispatch(UNUSED GSource* source, UNUSED GSourceFunc callback, UNUSED gpointer user_data) {
	XEvent e;
	ItemData *item;
	Display *display=((X11Source*)source)->dpy;

	while (XPending(display)) {
		XNextEvent(display, &e);

		g_message("Event type: %d", e.type);
		switch (e.type) {
		case Expose:
			item = host_item_find_window(e.xexpose.window);
			if(item)
				x11_draw_item(display, item);
			break;
		case ButtonPress:
			item = host_item_find_window(e.xbutton.window);
			if(!item)
				break;
			GError *error = NULL;
		    GVariant *params = g_variant_new("(ii)", e.xbutton.x_root, e.xbutton.y_root);
			g_dbus_proxy_call(item->proxy, "ContextMenu", params, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, &error);
			if(error) {
				g_warning("Failed to activate %s: %s", g_dbus_proxy_get_name(item->proxy), error->message);
				g_error_free(error);
			}
			break;
		default:
			break;
		}
	}
	return G_SOURCE_CONTINUE;
}

void x11source_finalize(UNUSED GSource *source) {
}

GSourceFuncs X11Funcs = {
		.prepare  = x11source_prepare,
		.check	= x11source_check,
		.dispatch = x11source_dispatch,
		.finalize = x11source_finalize
		};

GSource *x11source_new(Display *dpy) {
	GSource *source = g_source_new(&X11Funcs, sizeof(X11Source));

	((X11Source*)source)->dpy=dpy;
	int x11_fd = XConnectionNumber(dpy); 
	g_source_add_unix_fd(source, x11_fd, G_IO_IN | G_IO_HUP | G_IO_ERR );

	return source;
}
