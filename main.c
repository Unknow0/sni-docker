#include "x11source.h"
#include "watcher.h"
#include "host.h"
#include "x11.h"
#include "icons.h"
#include "utils.h"

#include <assert.h>
#include <signal.h>
#include <X11/Xutil.h>

static char *display_string = NULL;

static GMainLoop *loop;

static gboolean watcher = FALSE;

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
		g_main_loop_quit(loop);
	}
}

void parse_cmd_line(int argc, char **argv) {
	int i;
	gboolean help = FALSE;

	for (i = 1; i < argc; i++) {
		if (0 == strcasecmp(argv[i], "-display")) {
			++i;
			if (i < argc) {
				display_string = argv[i];
			} else { /* argument doesn't exist */
				g_printerr("-display requires a parameter\n");
				help = TRUE;
			}
		} else if (0 == strcasecmp(argv[i], "-watcher")) {
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
			g_print("	-display DISLPAY	The X display to connect to.\n");
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

int main(int argc, char **argv) {
	struct sigaction act;
	GSource *source;

	act.sa_handler = signal_handler;
	act.sa_flags = 0;
	sigaction(SIGSEGV, &act, NULL);
	sigaction(SIGPIPE, &act, NULL);
	sigaction(SIGFPE, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGHUP, &act, NULL);

	parse_cmd_line(argc, argv);

	display = XOpenDisplay(display_string);
	if (!display) 
		g_printerr("Unable to open Display %s. Exiting.\n", DisplayString(display_string));

	root = RootWindow(display, DefaultScreen(display));
	assert(root);

	icons_init();

	watcher_start();
	host_init();

	source = x11source_new(display);
	g_source_attach(source, NULL);
 
 	g_message("staring loop");
	loop=g_main_loop_new(NULL,0);
	g_main_loop_run(loop);
	g_message("Stoping");

	g_main_loop_unref(loop);
	watcher_destroy();
	host_destroy();

	XCloseDisplay(display);
	return 0;
}
