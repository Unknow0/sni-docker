#ifndef __host_item_h
#define __host_item_h

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>

extern gint icon_size;
extern gint border;

typedef struct {
	GDBusProxy *proxy;
	
	Window x11win;
	GtkWidget *window;
	GtkWidget *menu;
} ItemData;

void host_item_init();

void host_item_register(const gchar *item);

ItemData *host_item_find_window(Window win);

void host_item_unregister(const gchar *item);

void host_item_destroy();

#endif /* __host_item_h */
