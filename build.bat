
@echo off

if not exist build mkdir build

set CompileFlags=/nologo /FC /Zi /Od /Oi /std:c11 /I "..\extern\Vulkan-Headers\include" /GS- /Gs999999 /W4 /WX /wd4101 /wd4100 /wd4189
set LinkFlags=/incremental:no /opt:icf /opt:ref /nodefaultlib /subsystem:windows kernel32.lib user32.lib gdi32.lib

pushd build
call cl %CompileFlags% /DDebugBuild=1 /Fe:machine.exe "..\code\main.c" /link %LinkFlags%
popd
