# Dashcam

## Description

This project is a dashcam application that allows users to record and store video footage while driving.

A web interface supports real-time HTTP Live Stream and historical video playback.

For an optimal experience, the RPI should be setup as a Wireless Access Point.

## Build Dependencies
### On a new RPi, install these packages
```sh
sudo apt install cmake libcamera-dev libjsoncpp-dev uuid-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-libcamera libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x
```
- C++ Gstreamer Libs
- libcamera for RPI
- cmake
- Raspberry Pi Camera Module

## Running
- Make sure to have cmake and an up to date c++ compiler
1. Clone the repository.
2. `./build_and_install.sh`
     - Recordings directory: `/var/lib/dashcam/recordings`
     - `dashcam` is installed to `/usr/local/bin`
     - `dashcam.service` is installed to `/etc/systemd/system`

## Features
- Real-time video recording
- Clip saving and automatic file cleanup
- Video playback
- TODO Automatic event detection
- TODO Customizable settings

## Web Interface
- See upcoming github repo for web interface

### Dev Usage
- Pipe is opened in `/tmp/camrecorder.pipe`
- `$ echo save:300 >> /tmp/camrecorder.pipe` to save the last 300 seconds, or the most recent video if the configured VIDEO_DURATION is > 300 seconds.
- `$ echo stop >> /tmp/camrecorder.pipe` to stop cam server.
- Ctrl^C also kills the server cleanly too.



## Contributing

Contributions are welcome! Please follow the guidelines in the [CONTRIBUTING.md](./CONTRIBUTING.md) file.
