mkdir dependencies
cd dependencies

set platform=x64-windows

git clone https://github.com/microsoft/vcpkg
call vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg install dav1d:%platform%
.\vcpkg\vcpkg install opus:%platform%
.\vcpkg\vcpkg install libyuv:%platform%
.\vcpkg\vcpkg install sdl2:%platform%

cd ..