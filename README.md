# ueberzugpp

Drop in replacement for ueberzug written in C++

# Download

*ueberzugpp* is available in the [AUR](https://aur.archlinux.org/packages/ueberzugpp)

# Usage

The documentation is deferred at this point.

1. The only command currently provided is "layer" to make Ueberzug listen.
  - Layer accepts the silent [-s] option, to hide the textual output of the command.
  
    ```bash
    ueberzug layer -s
    ```

2. You can then feed Ueberzug with json objects to display an image or make it disappear.
  - json object to display the image / replace the image:
  
    ```json
    {"action":"add","identifier":"preview","max_height":0,"max_width":0,"path":"/path/image.ext","x":0,"y":0}
    ```
  
  The number values are COLUMNS and LINES of your terminal window, in TMUX it's relative to the size of the panels.

  - Don't display the image anymore:
  
    ```json
    {"action":"anything_else"}
    {"action":"remove","identifier":"preview"}
    ```

3. This project supports more filetypes by using [opencv](https://docs.opencv.org/4.7.0/d4/da8/group__imgcodecs.html#ga288b8b3da0892bd651fce07b3bbd3a56). But not SVGs, you have to convert and cache them first.

Until further documentation is available, it is easy to find examples of scripts for the Ueberzug using the json parser around the web.

# Build from source

## Dependencies

1. opencv
2. xcb-util-image
3. nlohmann-json
4. cli11
5. ninja/make

## Build instructions

1. Download and extract the latest release
2. Run the following commands in a terminal

```sh
$ git clone https://github.com/jstkdng/ueberzugpp.git
$ cd ueberzugpp
$ mkdir build && cd build
$ cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
$ ninja
```

after running these commands the resulting binary is ready to be used.

