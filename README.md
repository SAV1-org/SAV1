# SAV1

Library to enable simple and efficient playback of a WEBM file containing AV1 video and Opus audio tracks.

SAV1 provides a simple interface on top of file parsing, low level AV1 and Opus decoding, and tracking time for playback.
SAV1 allows the user to choose their preferred audio frequency / channels, and their preferred video pixelformat, and receive
data in those formats.

Currently SAV1 is in beta.

The library uses `dav1d` to efficiently decode video, `libopus` to efficiently decode audio-- vendors `libwebm` and `libyuv` for file parsing and color conversion respectively. The decoder and parsing modules are threaded internally so the top level API is non blocking
and efficient.

[Check out our documentation](https://sav1-org.github.io/SAV1/) 
and [our example programs](https://github.com/SAV1-org/SAV1/tree/main/examples)

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
