#!/bin/bash
pkg=`grep -E "^PACKAGE_NAME=" configure |sed -e "s/PACKAGE_NAME='\(.*\)'/\1/"`
ver=`grep -E "^PACKAGE_VERSION=" configure |sed -e "s/PACKAGE_VERSION='\(.*\)'/\1/"`
dh_make -s -n -c bsd -e SounOS@gmail.com 
