# SAV1

Library to enable simple and efficient playback of a WEBM file containing AV1 video and Opus audio tracks.

SAV1 provides a simple interface on top of file parsing, low level AV1 and Opus decoding, and tracking time for playback. It allows the user to choose their preferred audio frequency and number of channels, as well as their preferred video pixel format, in order to receive data in those formats.

Currently SAV1 is in beta.

SAV1 uses `dav1d` to efficiently decode video, `libopus` to efficiently decode audio, and vendors `libwebm` and `libyuv` for file parsing and color conversion respectively. The internal parsing and decoding modules run in separate threads so the top level API is non-blocking and efficient.

[Check out our documentation](https://sav1-org.github.io/SAV1/)
and [example programs](https://github.com/SAV1-org/SAV1/tree/main/examples)

## Install dependencies for build

### Windows

```
pip install ninja meson
install_windows_deps.bat
```

### MacOS

```
pip3 install meson ninja
brew install sdl2 dav1d opus
make all_dependencies
```

### Linux

If Python is not already installed:

```
sudo apt-get install python3 python3-pip python3-setuptools python3-wheel
```

Install the actual dependencies:

```
sudo apt-get install ninja-build libopus-dev libdav1d-dev libsdl2-dev
```

## Build with Meson

```
meson setup build
meson install -C build
```

Compiles sav1 and the example programs sav1play and sav1slideshow, installing them in the project root.

Our current build generates a video player called sav1play. Once, built, invoke it with a path to a test file to play that video. `sav1play test_files/ferris.webm` (for example). It also generates a frame-by-frame video player called sav1slideshow that is invoked with a video file the same way. `sav1slideshow test_files/presentation.webm` (for example).

## Build docs

-   Install doxygen

```
cd docs
doxygen doxyfile
```

## Conversion to AV1/opus webm

`ffmpeg -i [input file] -c:v libsvtav1 -q:v 0 -c:a libopus -format webm [output.webm]`

> If `libsvtav1` doesn't work for you, try `librav1e` or `libaom-av1`
