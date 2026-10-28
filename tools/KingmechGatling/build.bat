@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
cl /nologo /utf-8 /EHsc /MT /O2 /I C:\Users\tsyts\GitHub\2026_winter\EffekseerForDXLib %1.cpp /link /LIBPATH:C:\Users\tsyts\GitHub\2026_winter\EffekseerForDXLib /SUBSYSTEM:WINDOWS
