PREFIX = /usr/local
DOCDIR = $(PREFIX)/share/doc
LANGS = fr en

all:
	$(foreach lang,$(LANGS),$(MAKE) -C $(lang);)

clean:
	$(foreach lang,$(LANGS),$(MAKE) -C $(lang) clean;)

install:
	install -d $(DESTDIR)$(DOCDIR)/FreeMajor/manual/
	cp -rf resources $(DESTDIR)$(DOCDIR)/FreeMajor/manual/
	$(foreach lang,$(LANGS),install -D $(lang)/manual.html $(DESTDIR)$(DOCDIR)/FreeMajor/manual/$(lang)/manual.html;)
