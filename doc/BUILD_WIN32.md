Building on Windows
===================

The Windows build is not maintained: the old instructions targeted Qt 4,
OpenCV 2.4 and giflib 4 and no longer apply. The application now requires
Qt 6 and FFmpeg's libav* libraries.

An MSYS2/MinGW-w64 build is possible in principle with the following
packages, but it has not been tested after the Qt 6 port:

    pacman -S mingw-w64-x86_64-qt6-base \
              mingw-w64-x86_64-ffmpeg \
              mingw-w64-x86_64-giflib \
              mingw-w64-x86_64-cmake \
              mingw-w64-x86_64-ninja

Then configure and build from an MSYS2 MinGW shell:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build

The application needs the Qt 6, FFmpeg and giflib DLLs next to the
executable (or on PATH).
