# Thirty

**Thirty** is a [ClassiCube](https://classicube.net) (Minecraft Classic) server written in C.
It is available under the [AGPL v3](LICENCE.md).

## Features

- Support for Windows, Linux, macOS, BSDs
- Multiple [world generators](docs/map_generators.md)
- Some [Classic Protocol Extensions support](docs/cpe.md)
- WebSocket support

See [the `docs` folder](docs) for documentation.

## Building

Thirty requires [Meson](https://mesonbuild.com/) to generate build files, which in most cases will be for `ninja`.
[zlib](https://www.zlib.net/) is required for compressing the world for sending to clients and saving to disk.
[libcurl](https://curl.se/libcurl/) is used for heartbeats (updating the server on the ClassiCube server list).
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
apt install mercurial build-essential meson ninja-build zlib1g-dev libcurl4-gnutls-dev pkgconf
```

#### Alpine

```bash
apk add --update mercurial alpine-sdk zlib-dev samurai meson curl-dev
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

## Docker

An experimental Docker image is also available from this project's [container registry](https://dev.firestick.games/sean/thirty/container_registry).
It takes a volume, mounted at `/data`, which stores the configuration, levels, and logs.
Since no release of Thirty has been made yet, the only tag available is `dev`, which points to the latest changeset.

To use the Docker image, first choose a directory where you want the aforementioned data to be stored.
Then, copy the [default `settings.ini`](https://dev.firestick.games/sean/thirty/-/blob/branch/default/settings.ini) to that location, and edit it to your liking.

Use a Docker command like below, replacing `YOUR_DATA_DIRECTORY` with the directory on your real filesystem you chose to store the data.

```bash
docker run -it -v YOUR_DATA_DIRECTORY:/data registry.firestick.games/sean/thirty:dev
```

## openSUSE

A package for Thirty is available on the [openSUSE Build Service](https://build.opensuse.org/package/show/home:DrinkyBird/thirty) in my personal project.
This package is always built from the latest revision in the default branch from Mercurial.

Use the `opi` tool to install it. Select the `home:DrinkyBird` repository.

```
d349ee239370:/ # opi thirty
Searching repos for: thirty
1. thirty
Pick a number (0 to quit): 1
You have selected package name: thirty
1. home:DrinkyBird !                         | null.186.3862df3bcfa9     | x86_64
Pick a number (0 to quit): 1
```
