# ueberzugpp

Drop in replacement for ueberzug written in C++

# Download

*ueberzugpp* is available in the [AUR](https://aur.archlinux.org/packages/ueberzugpp)

# Build from source

## Dependencies

1. libvips
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
