#!/bin/bash
killall -9 sd ;gcc -o sd *.c -I utils/  -D_DEBUG -levbase -g && ./sd
