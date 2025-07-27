#ifndef __menu_h
#define __menu_h

#include <gio/gio.h>
#include <gtk/gtk.h>

GtkWidget *menu_create(const gchar *name, const gchar *menu_path);

#endif
