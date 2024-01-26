windres src\icon\iconadd.rc -O coff -o src\icon\iconadd.res
g++ src\textify.cpp -o Textify -std=c++17 -lstdc++fs src\icon\iconadd.res
pause