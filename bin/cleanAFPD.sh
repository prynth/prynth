#!/bin/bash
sudo find /home/pi/ -depth -name ".DS_Store" -exec rm {} \;
sudo find /home/pi/ -depth -name ".AppleDouble" -exec rm -Rf {} \;
sudo find /home/pi/ -depth -name ".AppleDB" -exec rm -Rf {} \;
sudo find /home/pi/ -depth -name ".AppleDesktop" -exec rm -Rf {} \;
sudo find /home/pi/ -depth -name "Temporary\ Items" -exec rm -Rf {} \;
sudo find /home/pi/ -depth -name "Network\ Trash\ Folder" -exec rm -Rf {} \;
