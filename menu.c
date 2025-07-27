#include "menu.h"
#include "utils.h"

#include <gtk/gtk.h>

static GtkWidget *menu_parse_layout(GVariant *children);

static GtkWidget *menu_parse_entry(GVariant *layout_node) {
	gint32 id;
	GVariant *props;
	GVariant *children;

	g_variant_get(layout_node, "(ia{sv}a(ia{sv}a(ia{sv})))", &id, &props, &children);

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GVariant *icon_var = g_variant_lookup_value(props, "icon-name", NULL);
	if (icon_var) {
		const gchar *icon_name = g_variant_get_string(icon_var, NULL);
		GtkWidget *image = gtk_image_new_from_icon_name(icon_name);
		gtk_box_append(GTK_BOX(box), image);
		g_variant_unref(icon_var);
	}


	// Lire le label
	const gchar *label="";
	GVariant *label_variant = g_variant_lookup_value(props, "label", NULL);
	if (label_variant) {
		label = g_variant_get_string(label_variant, NULL);
		GtkWidget *label_widget = gtk_label_new(label);
		gtk_box_append(GTK_BOX(box), label_widget);
		g_variant_unref(label_variant);
	}

	GtkWidget *btn = gtk_button_new();
	gtk_button_set_child(GTK_BUTTON(btn), box);

	if (g_variant_n_children(children) > 0) {
		GtkWidget *submenu = menu_parse_layout(children);
		GtkPopover *sub_pop = GTK_POPOVER(submenu);
		gtk_popover_set_has_arrow(sub_pop, TRUE);
		gtk_widget_set_hexpand(submenu, TRUE);

		gtk_widget_set_focusable(btn, TRUE);
		g_signal_connect_swapped(btn, "clicked", G_CALLBACK(gtk_popover_popup), submenu);
		gtk_widget_set_parent(submenu, btn);
	}

	g_variant_unref(props);
	g_variant_unref(children);
	return btn;
}

static GtkWidget *menu_parse_layout(GVariant *children) {
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	for (gsize i = 0; i < g_variant_n_children(children); i++) {
		GVariant *child = g_variant_get_child_value(children, i);
		GtkWidget *btn = menu_parse_entry(child);
		gtk_box_append(GTK_BOX(box), btn);
		g_variant_unref(child);
	}

	GtkWidget *popover = gtk_popover_new();
	gtk_popover_set_child(GTK_POPOVER(popover), box);
	return popover;
}


GtkWidget *menu_create(const gchar *name, const gchar *menu_path) {
	if(!name || !menu_path)
		return NULL;

	GError *error = NULL;
	GDBusConnection *co=g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	GVariant *params=g_variant_new("(iias)", 0, NULL, NULL);

	GVariant *result = g_dbus_connection_call_sync(co, name, menu_path, "com.canonical.dbusmenu", "GetLayout", params, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if(error) {
		g_warning("menu failed to get %s%s: %s", name, menu_path, error->message);
		g_error_free(error);
		return NULL;
	}

	GVariant *layout = NULL;
	g_variant_get(result, "(@(ia{sv}a(ia{sv}a{sv})))", &layout);

	GtkWidget *menu=menu_parse_layout(layout);

	g_variant_unref(layout);
	g_variant_unref(result);
	return menu;
}
