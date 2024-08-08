# Project Name

## Description

This project is a dashcam application that allows users to record and store video footage while driving. It provides features such as real-time video recording.

## Build Deps
- C++ OpenCV-libs
- cmake

## Running
- Make sure to have cmake and an up to date c++ compiler
1. Clone the repository.
2. `./build.sh`
3. `./build/dashcam`

## Usage
- Pipe is opened in `/tmp/camrecorder.pipe`
- `$ echo save:300 >> /tmp/camrecorder.pipe` to save the last 300 seconds, or the most recent video if the configured VIDEO_DURATION is > 300 seconds.
- `$ echo stop >> /tmp/camrecorder.pipe` to stop cam server.
- Ctrl^C also kills the server cleanly too.

## Features

- Real-time video recording
- Clip saving and automatic file cleanup
- TODO Automatic event detection
- TODO Video playback
- TODO Customizable settings

## Contributing

Contributions are welcome! Please follow the guidelines in the [CONTRIBUTING.md](./CONTRIBUTING.md) file.


## RPI CAM
`./build.sh -d -rpi`
### RPI deps
`sudo apt-get install libraspberrypi-dev`
```
git clone https://github.com/thaytan/gst-rpicamsrc
cd gst-rpicamsrc
./autogen.sh --prefix=/usr --libdir=/usr/lib/arm-linux-gnueabihf/
make
sudo make install
```
