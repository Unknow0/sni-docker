#include "utils.h" 
#include "host_item.h"
#include "x11.h"
#include "icons.h"

GPtrArray *items;


static void host_item_destroy_item(gpointer item) {
	ItemData *data=(ItemData*)item;

	g_signal_handlers_disconnect_by_data(data->proxy, NULL);
	g_object_unref(data->proxy);

	if(data->icon_x11)
		XFreePixmap(display, data->icon_x11);
	if(data->overlay_x11)
		XFreePixmap(display, data->overlay_x11);
	if(data->att_x11)
		XFreePixmap(display, data->att_x11);
	XDestroyWindow(display, data->win);

	g_free(data);
}

static gboolean host_item_find_name(gconstpointer a, gconstpointer b) {
	const gchar *n=g_dbus_proxy_get_name(((ItemData*)a)->proxy);
	const gchar *name=(const gchar*)b;
	const size_t i=strlen(n);
	if(!g_str_has_prefix(name, n))
		return FALSE;
	return strlen(name)>i && name[i]=='/';
}

static gchar *get_property_string(GDBusProxy *p, gchar *prop, GError **error) {
	gchar *retstr = NULL;

	GVariant *params=g_variant_new("(ss)", g_dbus_proxy_get_interface_name(p), prop);
	GVariant *val = g_dbus_proxy_call_sync(p, "org.freedesktop.DBus.Properties.Get", params, G_DBUS_CALL_FLAGS_NONE,
			-1, NULL, error);
	if(error && *error)
		return NULL;
	if(val == NULL)
		return NULL;
	GVariant *variant = NULL;
	g_variant_get(val, "(v)", &variant);
	if(variant != NULL) {
		retstr = g_variant_dup_string(variant, NULL);
		g_variant_unref(variant);
	}
	g_variant_unref(val);
	return retstr;
}

static void host_item_changed(GDBusProxy *p, gchar *sender_name, gchar *signal_name,
		GVariant *param, gpointer user_data) {
		ItemData *item=(ItemData*)user_data;
		// TODO update
	}

static void host_item_proxy_done(UNUSED GObject *source, GAsyncResult *res, UNUSED gpointer user_data) {
	GError *error = NULL;
	GDBusProxy *proxy = g_dbus_proxy_new_for_bus_finish(res, &error);
	if(!proxy) {
		g_warning("Failed to create proxy %s: %s", (char*)user_data, error->message);
		g_error_free(error);
		return;
	}

	gchar *id = get_property_string(proxy, "Id", &error);
	if(error) {
		g_error_free(error);
		const gchar *name = g_dbus_proxy_get_name(proxy);
		const gchar *path = g_dbus_proxy_get_object_path(proxy);

		g_warning("host item %s%s revert to org.kde.*", name, path);
		g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, NULL, name,
				path, "org.kde.StatusNotifierItem", NULL, host_item_proxy_done, NULL);
		g_object_unref(proxy);
		return;
		}

	ItemData *data=g_new0(ItemData, 1);
	g_signal_connect(proxy, "g-signal", G_CALLBACK(host_item_changed), data);


	g_message("host item connected %s", g_dbus_proxy_get_name_owner(proxy));
	gchar **names = g_dbus_proxy_get_cached_property_names(proxy);

	if (names) {
		for (int i = 0; names[i] != NULL; i++) {
			g_message("Property in cache: %s", names[i]);
		}
	} else {
		g_message("No cached properties available");
	}

	g_ptr_array_add(items, data);

	data->proxy = proxy;
	data->id = id;
	data->win = x11_create();

	data->category = get_property_string(proxy, "Category", NULL);
	data->title = get_property_string(proxy, "Title", NULL);
	data->status = get_property_string(proxy, "Status", NULL);

	if((data->icon_name = get_property_string(proxy, "IconName", NULL)) != NULL) {
		data->icon_path = icon_get(data->icon_name, icon_size);
		g_message("IconName %s", data->icon_path);
	}
//	GVariant *var = g_dbus_proxy_get_cached_property(proxy, "IconPixmap");
//	if(var!=NULL) {
//		g_message("has pixmap");
//	}
	if((data->overlay_name = get_property_string(proxy, "OverlayIconName", NULL)) != NULL)
		data->overlay_path = icon_get(data->overlay_name, icon_size);
	// OverlayIconPixmap

	if((data->att_name = get_property_string(proxy, "AttentionIconName", NULL)) != NULL)
		data->att_path = icon_get(data->att_path, icon_size);
	// AttentionIconPixmap

	// TODO load pixmap
	if(data->icon_path)
		data->icon_x11 = x11_icon_pixmap(data->icon_path);
	}

void host_item_register(const gchar *item) {
	g_message("host item register %s", item);
	gchar *name = g_strndup(item, g_strstr_len(item, -1, "/") - item);
	const gchar *path = g_strstr_len(item, -1, "/");

	g_message("host item register %s", item);
	g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, NULL, name,
			path, "org.freedesktop.StatusNotifierItem", NULL, host_item_proxy_done, NULL);
	g_free(name);
}

ItemData *host_item_find_window(Window win) {
	for(guint i = 0; i<items->len; i++) {
		ItemData *item = g_ptr_array_index(items, i);
		if(item->win==win)
			return item;
	}
	return NULL;
}

void host_item_unregister(const gchar *item) {
	guint i;

	if(g_ptr_array_find_with_equal_func(items, item, host_item_find_name, &i))
		g_ptr_array_remove_index_fast(items, i);
}

void host_item_init() {
	items=g_ptr_array_new();
	g_ptr_array_set_free_func(items, host_item_destroy_item);
}

void host_item_destroy() {
	if(items)
		g_ptr_array_free(items, TRUE);
}

