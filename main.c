#include "watcher.h"
#include "host.h"
#include "utils.h"
#include "host_item.h"

#include <assert.h>
#include <signal.h>
#include <X11/Xutil.h>
#include <gtk/gtk.h>

static gboolean watcher = FALSE;

extern GtkApplication *app;

void signal_handler(int signal) {
	switch (signal) {
	case SIGSEGV:
	case SIGFPE:
		g_printerr("Segmentation Fault or Critical Error encountered. "
							 "Dumping core and aborting.\n");
		abort();

	case SIGPIPE:
	case SIGTERM:
	case SIGINT:
	case SIGHUP:
		g_application_quit(G_APPLICATION(app));
	}
}

void parse_cmd_line(int argc, char **argv) {
	int i;
	gboolean help = FALSE;

	for (i = 1; i < argc; i++) {
		if (0 == strcasecmp(argv[i], "-watcher")) {
			watcher = TRUE;
		} else if (0 == strcasecmp(argv[i], "-border")) {
			++i;

			if (i < argc) {
				int b = atoi(argv[i]);
				if (b > 0) {
					border = b;
				} else {
					g_printerr("-border must be a value greater than 0\n");
					help = TRUE;
				}
			} else { /* argument doesn't exist */
				g_printerr("-border requires a parameter\n");
				help = TRUE;
			}
		} else if (0 == strcasecmp(argv[i], "-iconsize")) {
			++i;
			if (i < argc) {
				int s = atoi(argv[i]);
				if (s > 0) {
					icon_size = s;
				} else {
					g_printerr("-iconsize must be a value greater than 0\n");
					help = TRUE;
				}
			} else { /* argument doesn't exist */
				g_printerr("-iconsize requires a parameter\n");
				help = TRUE;
			}
		} else {
			if (argv[i][0] == '-')
				help = TRUE;
		}


		if (help) {
			/* show usage help */
			g_print("%s\n", argv[0]);
			g_print("Copyright 2017, Manuel <mdomlop@gmail.com>\n");
			g_print("Copyright 2003, Ben Jansens <ben@orodu.net>\n\n");
			g_print("Usage: %s [OPTIONS]\n\n", argv[0]);
			g_print("Options:\n");
			g_print("	-help						 Show this help.\n");
			g_print("	-border					 The width of the border to put around the\n"
							"										system tray icons. Defaults to 1.\n");
			g_print("	-vertical				 Line up the icons vertically. Defaults to\n"
							"										horizontally.\n");
			g_print("	-wmaker					 WindowMaker mode. This makes wmdocker a\n"
							"										fixed size (64x64) to appear nicely in\n"
							"										in WindowMaker.\n"
							"										Note: In this mode, you have a fixed\n"
							"										number of icons that wmdocker can hold.\n");
			g_print("	-iconsize SIZE		The size (width and height) to display\n"
							"										icons as in the system tray. Defaults to\n"
							"										24.\n");
			exit(1);
		}
	}
}

static void on_activate(GtkApplication *app, UNUSED gpointer user_data) {
	g_application_hold(G_APPLICATION(app));
}

int main(int argc, char **argv) {
	struct sigaction act;

	act.sa_handler = signal_handler;
	act.sa_flags = 0;
	sigaction(SIGSEGV, &act, NULL);
	sigaction(SIGPIPE, &act, NULL);
	sigaction(SIGFPE, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGHUP, &act, NULL);

	parse_cmd_line(argc, argv);

	watcher_start();
	host_init();

	gtk_init();
	GtkCssProvider *provider = gtk_css_provider_new();
	gtk_css_provider_load_from_string(provider, "* { background-color: rgba(0,0,0,0); color: white; }");

	GdkDisplay *display = gdk_display_get_default();
	gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

 	g_message("staring loop");
	app = gtk_application_new("io.github.unknow0.sni-dock", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
	g_application_run(G_APPLICATION(app), argc, argv);
	g_message("Stoping");

	g_object_unref(app);
	watcher_destroy();
	host_destroy();

	return 0;
}
