#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
#                                                                       #
# Change these values to customize your installation and build process  #
#                                                                       #
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#

# Change this PREFIX to where you want sni-tray to be installed
PREFIX=/usr/local
# Sets some flags for stricter compiling
CFLAGS=-pedantic -Wall -W -O --debug

#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
#                                                                 #
#  Leave the rest of the Makefile alone if you want it to build!  #
#                                                                 #
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#

PACKAGE=sni-docker
VERSION=1.5.1

target=sni-docker
sources=$(wildcard *.c)
headers=$(wildcard *.h)
extra=README COPYING version.h.in

pkg-deps=gio-2.0 x11 librsvg-2.0 gtk4

all: $(target) $(sources) $(headers)
	@echo Build Successful

$(target): $(sources:.c=.o)
	$(CC) $^ `pkg-config --libs $(pkg-deps)` -o $@

%.o: %.c
	$(CC) $(CFLAGS) `pkg-config --cflags $(pkg-deps)` -c $< 

install: all
	install -d -m 755 $(PREFIX)/bin
	install -Dm755 $(target) $(PREFIX)/bin/$(target)
	install -d -m 755 $(PREFIX)/share/licenses
	install -Dm644 COPYING $(PREFIX)/share/licenses/$(target)/LICENSE
	install -d -m 755 $(PREFIX)/share/doc
	install -Dm644 AUTHORS $(PREFIX)/share/doc/$(target)/AUTHORS
	install -Dm644 ChangeLog $(PREFIX)/share/doc/$(target)/ChangeLog
	install -Dm644 README.md $(PREFIX)/share/doc/$(target)/README.md
	install -Dm644 THANKS $(PREFIX)/share/doc/$(target)/THANKS

uninstall:
	rm -rf $(PREFIX)/$(target) $(PREFIX)/share/doc $(PREFIX)/share/licenses

clean:
	rm -rf pkg src
	rm -f *.pkg.tar.xz
	rm -rf .dist
	rm -f core *.o .\#* *\~ $(target)

distclean: clean
	rm -f version.h
	rm -f $(PACKAGE)-*.tar.gz

dist: Makefile $(sources) $(headers) $(extra)
	mkdir -p .dist/$(PACKAGE)-$(VERSION) && \
	cp $^ .dist/$(PACKAGE)-$(VERSION) && \
	tar -c -z -C .dist -f \
		$(PACKAGE)-$(VERSION).tar.gz $(PACKAGE)-$(VERSION) && \
	rm -rf .dist

love: $(sources)
	touch $^
