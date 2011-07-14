#!/bin/bash
prefix_path="/usr/local"
perl -i -p -e "s@/usr/local@$prefix_path@g" rc.lhttpd
install -c -m755  rc.lhttpd /etc/rc.d/init.d/lhttpd
chkconfig --add lhttpd
chkconfig --level 345 lhttpd on
install -c -m644 rc.lhttpd.ini /usr/local/etc/lhttpd.ini

