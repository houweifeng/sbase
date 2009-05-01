EXTRA_DIST = rc.lqftp  rc.lqftp.ini rc.lqftpd rc.lqftpd.ini lqftp.php setup.sh
install: install-ini

conftodir = "`echo $(sysconfdir)|sed -e 's|/usr/etc|/etc|'`"

sysconftodir = $(conftodir)

install-ini:
	@echo install lqftp.ini lqftpd to $(sysconftodir)
	@test -d $(sysconftodir)      || mkdir -p $(sysconftodir)
	@test -f $(sysconftodir)/lqftp.ini \
		|| cp -p $(top_srcdir)/doc/rc.lqftp.ini $(sysconftodir)/lqftp.ini
	@test -f $(sysconftodir)/lqftpd.ini \
		|| cp -p $(top_srcdir)/doc/rc.lqftpd.ini $(sysconftodir)/lqftpd.ini

