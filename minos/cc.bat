@echo off
rem For use with Windows

mingw32-make -j8
if not exist "out\minos.exe" (pause)

@echo on