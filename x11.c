#include "x11.h"

#include <gio/gio.h>
#include <X11/Xutil.h>
#include <cairo/cairo-xlib.h>
#include <librsvg/rsvg.h>

Display *display;
Window root;

gint border = 0;
gint icon_size = 24;
XWMHints wm_hints = { .initial_state = WithdrawnState, .flags = StateHint };

Window x11_create() {
	Window win=XCreateSimpleWindow(display, root, 0, 0, icon_size + border * 2 , icon_size + border * 2, 0, 0, 0);

    XSetWMHints(display, win, &wm_hints);
	XSetWindowBackgroundPixmap(display, win, ParentRelative);
	XSelectInput(display, win, ExposureMask|ButtonPressMask);
	XMapWindow(display, win);

	return win;
}

Pixmap x11_svg_to_pixmap(GFile *file) {
	GError *error = NULL;
	Pixmap pix= XCreatePixmap(display, root, icon_size, icon_size, DefaultDepth(display, 0));
	RsvgHandle *handle = rsvg_handle_new_from_gfile_sync (file, RSVG_HANDLE_FLAGS_NONE, NULL, &error);
	if (!handle) {
		g_error ("could not load: %s", error->message);
		return pix;
	}

	cairo_surface_t *surface = cairo_xlib_surface_create(display, pix, DefaultVisual(display, 0), icon_size, icon_size);
	cairo_t *cr = cairo_create(surface);
	RsvgRectangle viewport = {
		.x = 0.0,
		.y = 0.0,
		.width = icon_size,
		.height = icon_size,
	};

	if (!rsvg_handle_render_document (handle, cr, &viewport, &error)) {
		g_error ("could not render: %s", error->message);
		goto cleanup;
	}
cleanup:
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
	g_object_unref(handle);
	return pix;
}

Pixmap x11_icon_pixmap(gchar *path) {
	GFile *file=g_file_new_for_path(path);
	return x11_svg_to_pixmap(file);
}
