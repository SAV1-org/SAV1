# SAV1

Library to simply and efficiently decode a webm file containing AV1 video and opus audio tracks into frames and audio chunks to be displayed however the user chooses.

Currently SAV1 is in a pre alpha state.

Uses `dav1d` to efficiently decode video, `libopus` to efficiently decode audio-- vendors `libwebm` and `libyuv` for file parsing and color conversion respectively.

## Build instructions
Our current build generates a video player called dav3dplay. Once, built, invoke it with a path to a test file to play that video. `dav3dplay test_files/ferris.webm` (for example).

### Windows
```
install_windows_deps.bat
mingw32-make dav3dplay_win
```

### MacOS
```
brew install sdl2
brew install dav1d
brew install opus
make dav3dplay_mac
```

### Linux
Haven't yet tested on Linux, but it should be similar to Mac-- use a package manager to install sdl2, dav1d, and opus, then use the dav3dplay_mac command.

## Conversion to AV1/opus webm

`ffmpeg -i [input file] -c:v libsvtav1 -q:v 0 -c:a libopus -format webm [output.webm]`

> If `libsvtav1` doesn't work for you, try `librav1e` or `libaom-av1`
