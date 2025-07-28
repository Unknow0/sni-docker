#include "utils.h" 
#include "host_item.h"
#include "menu.h"

#include <gdk/x11/gdkx.h>
#include <gio/gio.h>
#include <X11/Xatom.h>

GPtrArray *items;

GtkApplication *app;

gint border = 0;
gint icon_size = 24;
static XWMHints wm_hints = { .initial_state = WithdrawnState, .flags = StateHint };

static void host_item_destroy_item(gpointer item) {
	ItemData *data=(ItemData*)item;

	g_signal_handlers_disconnect_by_data(data->proxy, NULL);
	g_object_unref(data->proxy);

	gtk_window_destroy(GTK_WINDOW(data->window));
	G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	Display *dpy = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
	G_GNUC_END_IGNORE_DEPRECATIONS
	XDestroyWindow(dpy, data->x11win);

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

static void host_item_menu_run(UNUSED GSimpleAction *action, GVariant *parameter, gpointer user_data) {
	ItemData *item=(ItemData*)user_data;
	gint id;
	g_variant_get(parameter, "(i)", &id);
	g_message("run %d", id);
}

static void host_item_clicked(GtkGestureClick *gesture, int npress, UNUSED double x, UNUSED double y, gpointer user_data) {
	ItemData *data=(ItemData*)user_data;
	guint button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));

	if (button == GDK_BUTTON_SECONDARY) {
		if(data->menu)
			gtk_popover_popup(GTK_POPOVER(data->menu));
		else
			; // call ContextMenu
	} else if (button == GDK_BUTTON_PRIMARY && npress==2) {
		g_print("Clic gauche sur la box\n");
	}
}

static void host_item_changed(GDBusProxy *p, gchar *sender_name, gchar *signal_name,
		GVariant *param, gpointer user_data) {
		ItemData *item=(ItemData*)user_data;
		// TODO update
	}

static gboolean host_item_window_on_map(GtkWidget *window, gpointer user_data) {
	ItemData *item = (ItemData*)user_data;

	GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(window));
	G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	Display *xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
	Window xwindow = GDK_SURFACE_XID(surface);
	G_GNUC_END_IGNORE_DEPRECATIONS

	XReparentWindow(xdisplay, xwindow, item->x11win, 0, 0);
	return FALSE;
}

static void host_item_build_window(ItemData *data) {
	GdkDisplay *gdk_display = gdk_display_get_default();

	G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	Display *dpy = gdk_x11_display_get_xdisplay(GDK_X11_DISPLAY(gdk_display));
	G_GNUC_END_IGNORE_DEPRECATIONS

	int screen = DefaultScreen(dpy);
	Window root = RootWindow(dpy, screen);

	Window x11win = XCreateSimpleWindow(dpy, root, 0, 0, icon_size + border * 2 , icon_size + border * 2, 0, 0, 0);
	data->x11win = x11win;

	XSetWMHints(dpy, x11win, &wm_hints);
	XSetWindowBackgroundPixmap(dpy, x11win, ParentRelative);
	XMapWindow(dpy, x11win);

	GtkWidget *window = gtk_application_window_new(app);
	data->window = window;

	gtk_window_set_default_size(GTK_WINDOW(window), icon_size+border*2, icon_size+border*2);
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

	gchar *icon_name=g_dbus_proxy_get_property_string(data->proxy, "IconName");
	GtkWidget *image = gtk_image_new_from_icon_name(icon_name);
	gtk_image_set_pixel_size(GTK_IMAGE(image), icon_size);

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_append(GTK_BOX(box), image);
	gtk_window_set_child(GTK_WINDOW(window), box);

	GtkGesture *click = gtk_gesture_click_new();
	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), 0); // tous les boutons
	g_signal_connect(click, "pressed", G_CALLBACK(host_item_clicked), data);
	gtk_widget_add_controller(box, GTK_EVENT_CONTROLLER(click));

	GMenu *model = menu_create(
		g_dbus_proxy_get_name(data->proxy),
		g_dbus_proxy_get_property_string(data->proxy, "Menu"));

	data->menu = gtk_popover_menu_new_from_model(G_MENU_MODEL(model));
	gtk_widget_set_parent(data->menu, data->window);

	GSimpleAction *action = g_simple_action_new("run", g_variant_type_new("(i)"));
	g_signal_connect(action, "activate", G_CALLBACK(host_item_menu_run), data);
	g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(action));

	g_signal_connect(window, "realize", G_CALLBACK(host_item_window_on_map), data);
	gtk_window_present(GTK_WINDOW(window));
}

static void host_item_proxy_done(UNUSED GObject *source, GAsyncResult *res, UNUSED gpointer user_data) {
	GError *error = NULL;
	GDBusProxy *proxy = g_dbus_proxy_new_for_bus_finish(res, &error);
	if(!proxy) {
		g_warning("Failed to create proxy %s: %s", (char*)user_data, error->message);
		g_error_free(error);
		return;
	}

	GVariant *params=g_variant_new("(ss)", g_dbus_proxy_get_interface_name(proxy), "Id");
	GVariant *val = g_dbus_proxy_call_sync(proxy, "org.freedesktop.DBus.Properties.Get", params, G_DBUS_CALL_FLAGS_NONE,
			-1, NULL, &error);
	if(val)
		g_variant_unref(val);
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

	g_message("host item connected %s%s", g_dbus_proxy_get_name(proxy), g_dbus_proxy_get_object_path(proxy));

	ItemData *data=g_new0(ItemData, 1);
	g_signal_connect(proxy, "g-signal", G_CALLBACK(host_item_changed), data);
	g_ptr_array_add(items, data);

	data->proxy = proxy;

	host_item_build_window(data);


//	data->menu = menu_create(g_dbus_proxy_get_name(data->proxy), g_dbus_proxy_get_property_string(proxy, "Menu"));
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
		if(item->x11win==win)
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

