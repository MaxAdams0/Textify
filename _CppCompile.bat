windres icon\iconadd.rc -O coff -o icon\iconadd.res
g++ src\textify.cpp -std=c++17 -lstdc++fs -O3 -o TextifyCpp icon\iconadd.res
pause
start TextifyCpp.exe