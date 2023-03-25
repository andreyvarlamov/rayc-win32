@echo off

set CompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DRAYC_INTERNAL=1 -DRAYC_SLOW=1 -FC -Z7
set LinkerFlags=-incremental:no -opt:ref
set LinkLibs=user32.lib Gdi32.lib winmm.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

cl %CompilerFlags% ..\code\win32_rayc.cpp -Fmwin32_rayc.map /link %LinkerFlags% %LinkLibs%

popd
