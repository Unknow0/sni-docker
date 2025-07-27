#ifndef __host_item_h
#define __host_item_h

#include <gio/gio.h>
#include <X11/Xlib.h>

typedef struct {
	GDBusProxy *proxy;
	Window win;
	gchar *category;
	gchar *id;
	gchar *title;
	gchar *status;
	gchar *icon_name;
	gchar *overlay_name;
	gchar *overlay_pixmap;
	gchar *att_name;
	Pixmap *att_pixmap;
	//TODO tooltip icon name, icon pixmap, title, description
	gboolean ismenu;
	GVariant *obj_path;

	// computed icon path
	gchar *icon_path;
	gchar *overlay_path;
	gchar *att_path;

	Pixmap icon_x11;
	Pixmap overlay_x11;
	Pixmap att_x11;
} ItemData;

void host_item_init();

void host_item_register(const gchar *item);

ItemData *host_item_find_window(Window win);

void host_item_unregister(const gchar *item);

void host_item_destroy();

#endif /* __host_item_h */
