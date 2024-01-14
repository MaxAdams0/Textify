@echo off
windres src\icon\iconadd.rc -O coff -o src\icon\iconadd.res
gcc src\textify.c -o Textify src\icon\iconadd.res
pause
start /B Textify.exe
