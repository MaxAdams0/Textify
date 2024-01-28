windres icon\iconadd.rc -O coff -o icon\iconadd.res
gcc src\textify.c -o TextifyC icon\iconadd.res
pause
start TextifyC.exe