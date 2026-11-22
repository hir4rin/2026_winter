@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
set INC=%~dp0..\..\EffekseerForDXLib
for %%f in (%*) do cl /nologo /utf-8 /wd4828 /EHsc /MT /O2 /I "%INC%" %%f.cpp /link /LIBPATH:"%INC%" /SUBSYSTEM:WINDOWS
