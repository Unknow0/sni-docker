#include "menu.h"
#include "utils.h"

#include <gtk/gtk.h>

static GMenu *menu_parse_child(GVariant *children);

static void menu_parse_entry(GMenu *menu, GVariant *layout_node, GMenu **section) {
	gint32 id;
	GVariant *props;
	GVariant *children;

	g_variant_get(layout_node, "(i@a{sv}@av)", &id, &props, &children);

	GVariant *type_variant = g_variant_lookup_value(props, "type", NULL);
	if (type_variant) {
		const gchar *type = g_variant_get_string(type_variant, NULL);
		if(g_strcmp0(type, "separator") == 0) {
			g_menu_append_section(menu, NULL, G_MENU_MODEL(*section));
			*section = g_menu_new();
			g_variant_unref(type_variant);
			return;
		}
		g_variant_unref(type_variant);
	}

	GMenuItem *item = g_menu_item_new(NULL, NULL);
	g_menu_item_set_action_and_target(item, "win.run", "(i)", id);
	GVariant *icon_var = g_variant_lookup_value(props, "icon-name", NULL);
	if (icon_var) {
		const gchar *icon_name = g_variant_get_string(icon_var, NULL);
		g_menu_item_set_icon(item, g_themed_icon_new(icon_name));
		g_variant_unref(icon_var);
	}

	GVariant *label_variant = g_variant_lookup_value(props, "label", NULL);
	if (label_variant) {
		const gchar *label = g_variant_get_string(label_variant, NULL);
		g_menu_item_set_label(item, label);
		g_variant_unref(label_variant);
	}

	if (g_variant_n_children(children) > 0)
		g_menu_item_set_submenu(item, G_MENU_MODEL(menu_parse_child(children)));

	g_variant_unref(props);
	g_variant_unref(children);
	g_menu_append_item(*section, item);
}

static GMenu *menu_parse_child(GVariant *children) {
	GMenu *menu = g_menu_new();
	GMenu *section=g_menu_new();
	for (gsize i = 0; i < g_variant_n_children(children); i++) {
		GVariant *child = g_variant_get_variant(g_variant_get_child_value(children, i));
		menu_parse_entry(menu, child, &section);
		g_variant_unref(child);
	}
	g_menu_append_section(menu, NULL, G_MENU_MODEL(section));
	return menu;
}


GMenu *menu_create(const gchar *name, const gchar *menu_path) {
	if(!name || !menu_path)
		return NULL;

	GError *error = NULL;
	GDBusConnection *co=g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	GVariant *params=g_variant_new("(iias)", 0, -1, NULL);

	GVariant *result = g_dbus_connection_call_sync(co, name, menu_path, "com.canonical.dbusmenu", "GetLayout", params, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if(error) {
		g_warning("menu failed to get %s%s: %s", name, menu_path, error->message);
		g_error_free(error);
		return NULL;
	}

	guint rev;
	GVariant *layout = NULL;
	g_variant_get(result, "(u@(ia{sv}av))", &rev, &layout);

	GVariant *children;
	g_variant_get(layout, "(ia{sv}@av)", NULL, NULL, &children);
	GMenu *menu = menu_parse_child(children);

	g_variant_unref(layout);
	g_variant_unref(children);
	g_variant_unref(result);
	return menu;
}
