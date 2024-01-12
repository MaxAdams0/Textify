@echo off
windres src\icon\iconadd.rc -O coff -o src\icon\iconadd.res
gcc -I"C:\msys64\mingw64\include\libavutil" -I"C:\msys64\mingw64\include\libavformat" -I"C:\msys64\mingw64\include\libavcodec" src\textify.c -o Textify src\icon\iconadd.res
pause
start /B Textify.exe
