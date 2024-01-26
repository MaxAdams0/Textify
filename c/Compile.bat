windres src\icon\iconadd.rc -O coff -o src\icon\iconadd.res
gcc src\textify2.c -o Textify2 src\icon\iconadd.res