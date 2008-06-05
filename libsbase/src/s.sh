#!/bin/bash
killall -9 sd ;gcc -o sd *.c -I utils/  -D_DEBUG -DHAVE_PTHREAD -lpthread -levbase -g && ./sd
