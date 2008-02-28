#!/bin/bash
prefix_path="/usr/local"
perl -i -p -e "s@/usr/local@$prefix_path@g" rc.lqftp rc.lqftpd
install -c -m755  rc.lqftp /etc/rc.d/init.d/lqftp
chkconfig --add lqftp
chkconfig --level 345 lqftp on
install -c -m644 rc.lqftp.ini /usr/local/etc/lqftp.ini

install -c -m755  rc.lqftpd /etc/rc.d/init.d/lqftpd
chkconfig --add lqftpd
chkconfig --level 345 lqftpd on
install -c -m644 rc.lqftpd.ini /usr/local/etc/lqftpd.ini

