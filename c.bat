windres icon\iconadd.rc -O coff -o icon\iconadd.res
gcc textify.c -o Textify icon\iconadd.res
pause
start Textify.exe