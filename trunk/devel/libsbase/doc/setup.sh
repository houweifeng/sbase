#!/bin/bash
prefix_path="/usr/local"
perl -i -p -e "s@/usr/local@$prefix_path@g" rc.lechod
install -c -m755  rc.lechod /etc/rc.d/init.d/lechod
chkconfig --add lechod
chkconfig --level 345 lechod on
install -c -m644 rc.lechod.ini /usr/local/etc/lechod.ini

