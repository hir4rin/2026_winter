@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
set INC=%~dp0..\..\EffekseerForDXLib
cl /nologo /utf-8 /std:c++20 /EHsc /MT /O2 hairlod.cpp
cl /nologo /utf-8 /wd4828 /EHsc /MT /O2 /I "%INC%" xdump.cpp /link /LIBPATH:"%INC%" /SUBSYSTEM:WINDOWS
cl /nologo /utf-8 /wd4828 /EHsc /MT /O2 /I "%INC%" hairsave.cpp /link /LIBPATH:"%INC%" /SUBSYSTEM:WINDOWS
cl /nologo /utf-8 /wd4828 /EHsc /MT /O2 /I "%INC%" hairanim.cpp /link /LIBPATH:"%INC%" /SUBSYSTEM:WINDOWS
