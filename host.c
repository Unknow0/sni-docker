#include "utils.h"
#include "icons.h"
#include "host_item.h"

#include <gio/gio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static gchar host[50] = "org.freedesktop.StatusNotifierHost-";
static const gchar watcher[] = "org.kde.StatusNotifierWatcher";
static const gchar watcher_path[] = "/StatusNotifierWatcher";

static gint id;
static guint watcher_id;
static GDBusProxy *proxy;

static void on_watch_sig_changed(UNUSED GDBusProxy *p, UNUSED gchar *sender_name, gchar *signal_name,
	GVariant *param, UNUSED gpointer user_data) {
	const gchar *item;
	if(g_strcmp0(signal_name, "StatusNotifierItemRegistered") == 0) {
		g_variant_get(param, "(&s)", &item);
		g_message("host %s: %s", signal_name, item);
		host_item_register(item);
	} else if(g_strcmp0(signal_name, "StatusNotifierItemUnregistered") == 0) {
		g_variant_get(param, "(&s)", &item);
		g_message("host %s: %s", signal_name, item);
		host_item_unregister(item);
	} else
		g_message("host %s: unknown signal", signal_name);

}

static void host_proxy_done(UNUSED GObject *source, GAsyncResult *res, gpointer user_data) {
	proxy = g_dbus_proxy_new_for_bus_finish(res, NULL);
	g_signal_connect(proxy, "g-signal", G_CALLBACK(on_watch_sig_changed), user_data);
	
	//initialize the list
	GVariant *items = g_dbus_proxy_get_cached_property(proxy, "RegisteredStatusNotifierItems");
	GVariantIter *it = g_variant_iter_new(items);
	GVariant *content;
	while((content = g_variant_iter_next_value(it)))
		host_item_register(g_variant_get_string(content, NULL));
	g_variant_iter_free(it);
	g_variant_unref(items);
}

static void host_registered(UNUSED GObject *source, UNUSED GAsyncResult *res, gpointer user_data) {
	g_message("host registered");
	g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, NULL, watcher,
			watcher_path, watcher, NULL, host_proxy_done, user_data);
}

static void watcher_appeared_handler(GDBusConnection *c, UNUSED const gchar *name,
	UNUSED const gchar *sender, gpointer user_data) {
	g_dbus_connection_call(c, watcher, watcher_path, watcher, "RegisterStatusNotifierHost",
			g_variant_new("(s)", host), NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, host_registered, user_data);
}

static void watcher_vanished_handler(UNUSED GDBusConnection *c, UNUSED const gchar *name, UNUSED gpointer user_data) {
	// TODO clear all icons
}

static void on_name_acquired(UNUSED GDBusConnection *c, UNUSED const gchar *name, UNUSED gpointer user_data) {
	watcher_id = g_bus_watch_name(G_BUS_TYPE_SESSION, watcher, G_BUS_NAME_WATCHER_FLAGS_NONE,
			watcher_appeared_handler, watcher_vanished_handler, user_data, NULL);
}
static void on_name_lost(UNUSED GDBusConnection *c, UNUSED const gchar *name, UNUSED gpointer user_data) {
	g_warning("Couldn't get name");
}

void host_init() {
	sprintf(host + strlen(host), "%ld", (long) getpid());

	host_item_init();

	id = g_bus_own_name(G_BUS_TYPE_SESSION, (const gchar *) host,
			G_BUS_NAME_OWNER_FLAGS_NONE, NULL, on_name_acquired, on_name_lost, NULL, NULL);
}

void host_destroy() {
	host_item_destroy();
	g_signal_handlers_disconnect_by_data(proxy, NULL);
	g_object_unref(proxy);
	if(id)
		g_bus_unown_name(id);
	if(watcher_id)
		g_bus_unwatch_name(watcher_id);
}
