EXTRA_DIST = libsbase.spec rc.lechod rc.lechod.ini rc.xhttpd rc.xhttpd.ini
install: install-ini

conftodir = "`echo $(sysconfdir)|sed -e 's|/usr/etc|/etc|'`"

sysconftodir = $(conftodir)

install-ini:
	@echo install xhttpd.ini lechod.ini to $(sysconftodir)
	@test -d $(sysconftodir)      || mkdir -p $(sysconftodir)
	@test -d $(sysconftodir)/init.d || mkdir -p $(sysconftodir)/init.d
	@test -f $(sysconftodir)/lechod.ini \
		|| cp -p $(top_srcdir)/doc/rc.lechod.ini $(sysconftodir)/lechod.ini
	@test -f $(sysconftodir)/xhttpd.ini \
		|| cp -p $(top_srcdir)/doc/rc.xhttpd.ini $(sysconftodir)/xhttpd.ini
	@test -f $(sysconftodir)/init.d/lechod \
		|| cp -p $(top_srcdir)/doc/rc.lechod $(sysconftodir)/init.d/lechod
	@test -f $(sysconftodir)/init.d/xhttpd \
		|| cp -p $(top_srcdir)/doc/rc.xhttpd $(sysconftodir)/init.d/xhttpd
	@perl -i -p -e "s@/usr/local/bin@$(prefix)/bin@g" \
		$(sysconftodir)/init.d/xhttpd $(sysconftodir)/init.d/lechod
	@perl -i -p -e "s@/usr/local/etc@$(sysconftodir)@g" \
		$(sysconftodir)/init.d/xhttpd  $(sysconftodir)/init.d/lechod

