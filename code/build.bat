#echo

mkdir ..\build
pushd ..\build


set CommonCompilerFlags= -Zi -O2 -W4 -wd4201 -wd4100 -wd4189  -wd4127 -nologo /INCREMENTAL

call "c:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set path = d:\handmadehero\misc;%path%

cl %CommonCompilerFlags% d:\helloworld\code\win32_platform.cpp user32.lib Gdi32.lib Opengl32.lib

popd
