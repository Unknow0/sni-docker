#ifndef __utils_h
#define __utils_h

#include <gio/gio.h>

#ifdef __GNUC__
#  define UNUSED __attribute__((__unused__))
#else
#  define UNUSED 
#endif

gchar *g_dbus_proxy_get_property_string(GDBusProxy *p, const gchar *prop);

#endif /* __utils_h */
