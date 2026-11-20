# QGifer

QGifer is a tool for extracting part of a video to an animated GIF file.

http://sourceforge.net/projects/qgifer/

    Author:	Lukasz Chodyla <chodak166@op.pl>
    Author:	Arkadii Ivanov <apkawa@gmail.com>
    Author:	Ruslan V. Uss <unclerus@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Niniejszy program jest wolnym oprogramowaniem; możesz go
    rozprowadzać dalej i/lub modyfikować na warunkach Powszechnej
    Licencji Publicznej GNU, wydanej przez Fundację Wolnego
    Oprogramowania - według wersji 3 tej Licencji lub (według twojego
    wyboru) którejś z późniejszych wersji.

    Niniejszy program rozpowszechniany jest z nadzieją, iż będzie on
    użyteczny - jednak BEZ JAKIEJKOLWIEK GWARANCJI, nawet domyślnej
    gwarancji PRZYDATNOŚCI HANDLOWEJ albo PRZYDATNOŚCI DO OKREŚLONYCH
    ZASTOSOWAŃ. W celu uzyskania bliższych informacji sięgnij do
    Powszechnej Licencji Publicznej GNU.

    Z pewnością wraz z niniejszym programem otrzymałeś też egzemplarz
    Powszechnej Licencji Publicznej GNU (GNU General Public License);
    jeśli nie - napisz do Free Software Foundation, Inc., 59 Temple
    Place, Fifth Floor, Boston, MA  02110-1301  USA


## Requirements

 * Qt version 6.5 or higher (Core, Gui, Widgets).
 * FFmpeg development libraries (libavformat, libavcodec, libavutil, libswscale) version 5.0 or higher.
 * giflib version 5.1 or higher.
 * CMake version 3.21 or higher and a C++17 compiler.


## Building on Debian-based distributions

The build is tested on Debian 13 (trixie) with Qt 6.11. Debian 13 and Ubuntu 25.04
(or newer) are required, because earlier releases ship Qt 6.4 while QGifer needs Qt 6.5+.

Install the build dependencies:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build \
    qt6-base-dev qt6-base-dev-tools qt6-svg-plugins \
    libavformat-dev libavcodec-dev libavutil-dev libswscale-dev \
    libgif-dev
```

`qt6-base-dev-tools` provides `moc`, `uic` and `rcc`, and `qt6-svg-plugins` is
needed to load the SVG icons. To regenerate the translation files (`./tsupdate`)
also install `qt6-l10n-tools` (`lupdate6`, `lrelease6`).

Configure and build:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Run the freshly built binary without installing:

```bash
./build/qgifer
```

Install system-wide (the default prefix is `/usr/local`; add
`-DCMAKE_INSTALL_PREFIX=/usr` to the configure step to install into `/usr`):

```bash
sudo cmake --install build
```

Two dialogs call external tools at runtime: `ffmpeg` (video conversion) and
ImageMagick `convert` (GIF optimization). They are optional:

```bash
sudo apt install ffmpeg imagemagick
```

To build a `.deb` package see [Build debian package](#build-debian-package).


## Compilation and installation

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
sudo cmake --install build
```

Add `-DQUIET_MODE=ON` to disable the application's debug output.
For a debug build use `-DCMAKE_BUILD_TYPE=Debug`.


## Build debian package
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && cpack -G DEB
```
