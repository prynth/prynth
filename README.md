# Prynth
The Prynth framework uses the Raspberry Pi (RPi) for sound synthesis and the Teensy microcontroller for sensor signal acquisition. The Teensy communicates with the RPi through an add-on board, which in turn connects to up to 10 multiplexer boards for a total 80 analog sensor connections (pots, resistors or switches). It also includes connections for digital sensors through I2C and SPI buses.

With the Prynth framework, the RPi boots into a system that automatically runs a web service with a full code editor for the SuperCollider programing language.

For more information please visit the [Prynth website](https://prynth.github.io/).

# Installation

## Prepare the OS

- Flash the latest Raspbian Lite image (https://www.raspberrypi.org/software/operating-systems/#raspberry-pi-os-32-bit) to your microSD card.
- Mount the card on your computer and create an file named ssh located at the root of the boot folder. In Linux:\
`
touch ssh
`
- If you want connect straight to your WiFi, you can also create a wpa_supplicant.conf file on the same booh folder. The wpa_supplicant.conf file should contain:
```
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1
country=GB
network={
    ssid="your_real_wifi_ssid"
    scan_ssid=1
    psk="your_real_password"
    key_mgmt=WPA-PSK
}
```
- Boot Raspberry Pi.

## Expand filesystem
- SSH to Raspberry Pi and run the Raspbian configuration program.
  - `sudo raspi-config`
- Expand filesystem: "7 Advanced Options" > "A1 Expand Filesystem".
- Reboot.

## Upgrade and install additional libraries
- SSH to Raspberry Pi.
- Update libraries.
  - `sudo apt-get update`
  - `sudo apt-get upgrade`
- Install additional libraries required for Jack and SuperCollider.
  - `sudo apt-get -y install vim alsa-base libicu-dev libasound2-dev libsamplerate0-dev libsndfile1-dev libreadline-dev libxt-dev libudev-dev libavahi-client-dev libfftw3-dev cmake git libcwiid-dev`
- Reboot.

## Compile and install Jack
- `git clone git://github.com/jackaudio/jack2 --depth 1`
- `cd jack2`
- `./waf configure --alsa --libdir=/usr/lib/arm-linux-gnueabihf/`
- `./waf build`
- `sudo ./waf install`
- `sudo ldconfig`
- `cd .. && rm -rf jack2`

## Set limits.conf
- `sudo sed -i '/# End of file/ i @audio - memlock 256000\n@audio - rtprio 75\n' /etc/security/limits.conf`

## Compile and install Supercollider
- `git clone --recursive git://github.com/supercollider/supercollider`
- `cd supercollider`
- `git submodule init && git submodule update`
- `mkdir build && cd build`
- `cmake -L -DCMAKE_BUILD_TYPE="Release" -DBUILD_TESTING=OFF -DSSE=OFF -DSSE2=OFF -DSUPERNOVA=OFF -DNATIVE=OFF -DSC_WII=OFF -DSC_IDE=OFF -DSC_QT=OFF -DSC_ED=OFF -DSC_EL=OFF -DSC_VIM=OFF ..`
- `make -j 4`
- `sudo make install`
- `sudo ldconfig`
- `cd ../.. && rm -rf supercollider`
- Reboot.

## Test Jack and SuperCollider
- Connect your soundcard (USB or I2S)
- Run Jack with sound device 1 (-dhw:1)
  - `jackd -P75 -dalsa -dhw:1 -p1024 -n3 -s -r44100 &`
- Run SuperCollider
  - `sclang`
- Inside the sclang shell, boot the server and play an event. You should hear a sound.
  - `s.boot`
  - `(dur:3).play`
- Quit the server and exit sclang
  - `s.quit`
  - `0.exit`

## Setup UART for Prynth boards and enable overclocking
- SSH to Raspberry Pi.
- Run Raspiconfig:
  - `sudo raspi-config`
- Disable shell over serial and enable hardware port:
  - 5 Interfacing Options > P6 Serial
  - "Would you like a login shell to be accessible over serial?" > No
  - "Would you like the serial port hardware to be enabled?" > Yes
- Reboot.
- SSH to Raspberry Pi.
- Additional boot settings (disable Bluetooth and enable overclocking)
  - `sudo sed -i '$ a dtoverlay=pi3-disable-bt\nforce_turbo=1\n' /boot/config.txt`

## Clone prynth in the pi home folder
- `cd ~`
- `git clone https://github.com/prynth/prynth.git`  

## Install Node.js, liblo and WiringPi
- `sudo apt-get install nodejs liblo7 liblo-dev wiringpi`

## Recompile the pbridge application
- `gcc -o /home/pi/prynth/pbridge/pbridge /home/pi/prynth/pbridge/src/pbridge.c -lwiringPi -llo`

## Enable Prynth at startup
- Launch crontab
  - `crontab -e`
- Choose your preferred text editor (nano is easier)
- Add the following line at the end of the file:
  - `@reboot sudo nodejs /home/pi/prynth/server/bin/www`
- Reboot.

# Notes
For more information please refer to the [Prynth documentation](https://prynth.github.io/doc/). Prynth is also distributed as a ready-to-use image file hosted on the [Downloads](https://prynth.github.io/create/downloads.html) section.
