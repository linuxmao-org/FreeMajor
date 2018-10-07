
PREFIX = /usr/local
DOCDIR = $(PREFIX)/share/doc
LANGS = fr

all:
	$(foreach lang,$(LANGS),$(MAKE) -C $(lang))

clean:
	$(foreach lang,$(LANGS),$(MAKE) -C $(lang) clean)

install:
	install -d $(DESTDIR)$(DOCDIR)/gmajctl/manual/
	cp -rf resources $(DESTDIR)$(DOCDIR)/gmajctl/manual/
	$(foreach lang,$(LANGS),install -D $(lang)/manual.html $(DESTDIR)$(DOCDIR)/gmajctl/manual/$(lang)/manual.html)
