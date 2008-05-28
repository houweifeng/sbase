#!/bin/bash
gcc -o sd *.c -I utils/  -D_DEBUG -levbase -g && ./sd
