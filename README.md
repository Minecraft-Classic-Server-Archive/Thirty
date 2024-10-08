# Thirty

**Thirty** is a [ClassiCube](https://classicube.net) (Minecraft Classic) server written in C.

## Features

- Support for Windows, Linux, macOS, BSDs
- Multiple world generators
- Some Classic Protocol Extensions support
- WebSocket support

See [the wiki](https://dev.firestick.games/sean/thirty/-/wikis/home) for documentation.

## Building

Thirty requires [Meson](https://mesonbuild.com/) to generate build files, which in most cases will be for `ninja`.
[zlib](https://www.zlib.net/) is required for compressing the world for sending to clients and saving to disk.
[Mercurial](https://mercurial-scm.org) is used to keep the source code and is required to clone the repository.

On Windows, compiling with [MSYS2](https://www.msys2.org/) is recommended.
The MinGW64 and clang64 environments work; clang seems to work better for debugging.

### Install packages

#### MSYS2

```bash
pacman -S mercurial $MINGW_PACKAGE_PREFIX-toolchain $MINGW_PACKAGE_PREFIX-meson $MINGW_PACKAGE_PREFIX-ninja $MINGW_PACKAGE_PREFIX-zlib
```

#### Debian

```bash
apt install mercurial build-essential meson ninja-build zlib1g-dev
```

#### Alpine

```bash
apk add --update mercurial alpine-sdk zlib-dev samurai meson
```

### Clone repository

```bash
hg clone https://dev.firestick.games/sean/thirty
```

### Build

```bash
meson setup build
cd build
ninja
```

An executable named `thirty` will be output to the build directory.

### Install

You can use [`meson install`] to install Thirty to `PREFIX/bin/thirty` and its config to `PREFIX/etc/thirty.ini`.

If unspecified, `PREFIX` will likely be `/usr/local`.
