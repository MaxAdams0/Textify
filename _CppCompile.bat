windres icon\iconadd.rc -O coff -o icon\iconadd.res
g++ src\cpp\textify.cpp -o TextifyCpp -std=c++17 -lstdc++fs icon\iconadd.res
pause
start TextifyCpp.exe