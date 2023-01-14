set installroot=dependencies
set platform=x64-windows

git clone https://github.com/microsoft/vcpkg
call vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg install dav1d:%platform% --x-install-root=%installroot%
.\vcpkg\vcpkg install opus:%platform% --x-install-root=%installroot%
#.\vcpkg\vcpkg install libyuv:%platform% --x-install-root=%installroot%
.\vcpkg\vcpkg install sdl2:%platform% --x-install-root=%installroot%

robocopy "dependencies/x64-windows/bin/" . *.dll
