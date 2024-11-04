#!/bin/bash
gcc -Wall -Wextra -Werror -std=c99 -pedantic -O2  ANSWER.c
./a.out
echo "The exit status of the program is: $?"