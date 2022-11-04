# SAV1

Library to simply and efficiently decode a webm file containing AV1 video and opus audio tracks into frames and audio chunks to be displayed however the user chooses.

Uses `dav1d` to efficiently decode video, vendors `libwebm` and `libyuv` for file parsing and color conversion respectively.

## Conversion to AV1/opus webm

`ffmpeg -i [input file] -c:v libsvtav1 -q:v 0 -c:a libopus -format webm [output.webm]`

> If `libsvtav1` doesn't work for you, try `librav1e` or `libaom-av1`

## Resources

Drive link for files:
https://drive.google.com/drive/folders/1Y8lpvJU0lKn1RZTbXIF70x55Dnw7BMcM?usp=sharing