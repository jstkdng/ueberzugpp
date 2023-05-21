# Überzug++

Überzug++ is a command line utility written in C++ which allows to draw images on terminals by using X11 child windows, sixels, kitty and iterm2..

This project intends to be a drop-in replacement for the now defunct [ueberzug](https://github.com/seebye/ueberzug) project. If some tool doesn't work,
feel free to open an issue.

Advantages over w3mimgdisplay and ueberzug:

- support for MacOS
- no race conditions as a new window is created to display images
- expose events will be processed, so images will be redrawn on switch workspaces
- tmux support on X11
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
- [ytfzf](https://github.com/pystardust/ytfzf)
- ÜberzugPP is a drop in replacement for Ueberzug, so applications that worked with ueberzug should work out of the box with this project.

# Download

Arch Linux: [AUR](https://aur.archlinux.org/packages/ueberzugpp)

MacOS: [Homebrew](https://github.com/jstkdng/ueberzugpp/blob/master/homebrew/ueberzugpp.rb)

# Usage

1. Ueberzugpp provides two commands, `layer` and `tmux`. `layer` is used to send
commands to ueberzugpp, `tmux` is used internally. 

- Layer accepts the following options
  
```bash
$ ueberzug layer -h
Display images on the terminal.
Usage: ueberzug layer [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  -s,--silent                 Print stderr to /dev/null.
  --use-escape-codes [0]      Use escape codes to get terminal capabilities.
  --no-stdin                  Do not listen on stdin for commands.
  --no-cache                  Disable caching of resized images.
  --no-opencv                 Do not use OpenCV, use Libvips instead.
  -o,--output TEXT:{x11,sixel,kitty,iterm2}
                              Image output method
  -p,--parser                 **UNUSED**, only present for backwards compatibility.
  -l,--loader                 **UNUSED**, only present for backwards compatibility.
```

2. You can also configure ueberzugpp with a json file. The file should be located
on `$XDG_CONFIG_HOME/ueberzugpp/config.json`, in case XDG_CONFIG_HOME isn't set,
ueberzugpp will look for the configuration at `~/.config/ueberzugpp/config.json`

Application flags have precedence over the configuration file.
The configuration file should have this format.

```json
{
    "layer": {
        "silent": true,
        "use-escape-codes": false,
        "no-stdin": false,
        "output": "sixel"
    }
}
```

The most helpful is the `output` variable as that can be used to force
ueberzugpp to output images with a particular method.

3. By default, commands are sent to ueberzug++ through stdin, this is enough in
some cases. In some terminals and application combinations (e.g. ranger + wezterm + zellij)
using stdin to send commands doesn't work properly or ueberzug++ could fail to
start altogether. In those cases, the user may send commands to ueberzug++ through
a unix socket. By default, ueberzug++ will listen to commands on /tmp/ueberzug_{$USER}.sock.

New software is encouraged to use sockets instead of stdin as they cover more cases.

4. You can then feed Ueberzug with json objects to display an image or make it disappear.
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

This project uses C++20 features so you must use a recent compiler. GCC 10.1 is
the minimum supported version.

## Required dependencies

Must be installed in order to build.

- cmake (3.18 <= )
- zeromq
- libvips
- libsixel
- chafa (1.6 <= )
- openssl
- tbb

## Downloadable dependencies

Required for building, if they are not installed, they will be downloaded
and included in the binary.

- nlohmann-json
- cli11
- spdlog
- fmt
- cppzmq

## Optional dependencies

Not required for building, can be disabled/enabled using flags.

- opencv
- xcb-util-image
- turbo-base64

## Build instructions

1. Download and extract the latest release.

2. Choose feature flags

The following feature flags are available:

ENABLE_OPENCV (ON by default)

ENABLE_X11 (ON by default)

ENABLE_TURBOBASE64 (OFF by default)

You may use any of them when building the project, for example:

- Compile with default options

```sh
git clone https://github.com/jstkdng/ueberzugpp.git
cd ueberzugpp
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

- Disable X11 and OpenCV support

```sh
git clone https://github.com/jstkdng/ueberzugpp.git
cd ueberzugpp
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_X11=OFF -DENABLE_OPENCV=OFF ..
cmake --build .
```

- Enable support for Turbo-Base64

```sh
git clone https://github.com/jstkdng/ueberzugpp.git
cd ueberzugpp
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_TURBOBASE64=ON ..
cmake --build .
```

after running these commands the resulting binary is ready to be used.

