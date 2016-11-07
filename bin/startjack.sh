#!/bin/bash
sudo /usr/local/bin/jackd -P75 -dalsa -dhw:1 -r 44100 -p512 -n4 -s &
