# Überzug++

Überzug++ is a command line utility written in C++ which allows to draw images on terminals by using child windows or using sixel on supported terminals. 

This is a drop-in replacement for the now defunct [ueberzug](https://github.com/seebye/ueberzug) project.

Advantages over w3mimgdisplay and ueberzug:

- X11 and sixel support on supported terminals
- no race conditions as a new window is created to display images
- expose events will be processed, so images will be redrawn on switch workspaces
- tmux support
- terminals without the WINDOWID environment variable are supported
- chars are used as position - and size unit
- No memory leak (usage of smart pointers)
- A lot of image formats supported (through opencv and libvips).
- GIF and animated WEBP support on X11 and Sixel
- Fast image downscaling (through opencv and opencl)
- Cache resized images for faster viewing

# Applications that use Überzug++

- [Ranger](https://github.com/ranger/ranger)
- [Termusic](https://github.com/tramhao/termusic/)
- ÜberzugPP is a drop in replacement for Ueberzug, so applications that worked with ueberzug should work out of the box with this project.

# Download

*ueberzugpp* is available in the [AUR](https://aur.archlinux.org/packages/ueberzugpp)

# Usage

1. The only command currently provided is "layer" to make Ueberzug listen.
  - Layer accepts the silent [-s] option, to hide the textual output of the command.
  
    ```bash
    ueberzug layer -s
    ```

2. You can then feed Ueberzug with json objects to display an image or make it disappear.
  - json object to display the image:
  
    ```json
    {"action":"add","identifier":"preview","max_height":0,"max_width":0,"path":"/path/image.ext","x":0,"y":0}
    ```
  
  The number values are COLUMNS and LINES of your terminal window, in TMUX it's relative to the size of the panels.

  - Don't display the image anymore:
  
    ```json
    {"action":"remove","identifier":"preview"}
    ```

# Build from source

This project uses C++20 features so you must use a recent compiler.

## Dependencies

- cmake version 3.18 ≤
- opencv
- libvips
- xcb-util-image
- nlohmann-json
- cli11
- libsixel
- botan

## Build instructions

1. Download and extract the latest release
2. Run the following commands in a terminal

```sh
$ git clone https://github.com/jstkdng/ueberzugpp.git
$ cd ueberzugpp
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ cmake --build .
```

after running these commands the resulting binary is ready to be used.

