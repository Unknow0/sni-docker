#include "utils.h"

gchar *g_dbus_proxy_get_property_string(GDBusProxy *proxy, const gchar *prop) {
    GVariant *v = g_dbus_proxy_get_cached_property(proxy, prop);
    gchar *str = g_variant_dup_string(v, NULL);
    g_variant_unref(v);
	return str;
}
