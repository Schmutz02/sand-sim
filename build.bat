@echo off

IF NOT EXIST build mkdir build
pushd build

cl -Zi -nologo -Oi -MTd -DDEBUG ../main.c -link -incremental:no -out:sand-d.exe gdi32.lib user32.lib
cl -nologo -Oi -O2 -MT ../main.c -link -incremental:no -out:sand.exe gdi32.lib user32.lib

popd
