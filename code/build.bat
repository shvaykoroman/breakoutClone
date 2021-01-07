#echo

mkdir ..\build
pushd ..\build


set CommonCompilerFlags= -Zi -Od -W4 -wd4201 -wd4100 -wd4189  -wd4127 -nologo /INCREMENTAL

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\auxiliary\build\vcvarsall.bat" x64

cl %CommonCompilerFlags% "C:\Program Files (x86)\Projects\breakout\code\win32_platform.cpp" user32.lib Gdi32.lib Opengl32.lib

popd
