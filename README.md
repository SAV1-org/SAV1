# SAV1

Library to simply and efficiently decode a webm file containing AV1 video and opus audio tracks into frames and audio chunks to be displayed however the user chooses.

Currently SAV1 is in beta.

Uses `dav1d` to efficiently decode video, `libopus` to efficiently decode audio-- vendors `libwebm` and `libyuv` for file parsing and color conversion respectively.

## Install dependencies for build
### Windows
```
pip install meson
install_windows_deps.bat
mingw32-make all_dependencies (<- run that one in bash)
```
+ Download and install ninja from its website

### MacOS
```
pip3 install meson
brew install sdl2 dav1d opus ninja
make all_dependencies
```

## Build with Meson
```
meson setup build
meson install -C build
```
Compiles sav1 and sav1play, installing them in the project root.

Our current build generates a video player called sav1play. Once, built, invoke it with a path to a test file to play that video. `sav1play test_files/ferris.webm` (for example).

## Build docs
+ Install doxygen

```
cd docs
doxygen doxyfile
```

## Conversion to AV1/opus webm

`ffmpeg -i [input file] -c:v libsvtav1 -q:v 0 -c:a libopus -format webm [output.webm]`

> If `libsvtav1` doesn't work for you, try `librav1e` or `libaom-av1`
