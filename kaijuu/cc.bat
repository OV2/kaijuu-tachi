@echo off
rem For use with Windows

mingw32-make -j8
if not exist "out\kaijuu64.dll" (pause) else if not exist "out\kaijuu64.exe" (pause)

@echo on