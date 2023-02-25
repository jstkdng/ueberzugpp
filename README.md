# Überzug++

Überzug++ is a command line utility written in C++ which allows to draw images on terminals by using child windows or using sixel on supported terminals. 

This project intends to be a drop-in replacement for the now defunct [ueberzug](https://github.com/seebye/ueberzug) project. If some tool doesn't work,
feel free to open an issue.

Advantages over w3mimgdisplay and ueberzug:

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
  - Layer accepts the following options
  
    ```bash
    $ ueberzug layer -h
    Usage: ueberzug layer [OPTIONS]
    
    Options:
      -h,--help                   Print this help message and exit
      -s,--silent                 Print stderr to /dev/null
      --tcp                       Send commands through a tcp socket on port 56988
      --tcp-port INT Needs: --tcp Change tcp port used
      --x11 Excludes: --sixel     Force X11 output
      --sixel Excludes: --x11     Force sixel output
      -p,--parser                 **UNUSED**, only present for backwards compatibility
      -l,--loader                 **UNUSED**, only present for backwards compatibility
    ```

2. By default, commands are sent to ueberzug++ through stdin, this is enough in
some cases. In some terminals and application combinations (e.g. ranger + wezterm + zellij)
using stdin to send commands doesn't work properly or ueberzug++ could fail to
start altogether. In those cases, the user may send commands to ueberzug++ through
a TCP socket. By default, ueberzug++ will listen to commands on port 56988, the user
may change this port by passing the `--tcp-port` option.

New software is encouraged to use tcp instead of stdin as tcp covers most cases.

3. You can then feed Ueberzug with json objects to display an image or make it disappear.
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
- spdlog
- fmt
- zeromq
- cppzmq (in some distributions)

## Build instructions

1. Download and extract the latest release
2. Run the following commands in a terminal

```sh
git clone https://github.com/jstkdng/ueberzugpp.git
cd ueberzugpp
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

after running these commands the resulting binary is ready to be used.

