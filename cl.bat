@echo off
cls
set flags=-fcompare-debug-second -Wall -Wformat=0 -static-libgcc -g -O2 -Wno-unused-variable -Wno-unused-function 
gcc %flags% -c *.c
gcc *.o -o p -lopengl32 -lglfw3 -lglew32 -lgdi32 -lm
del /f *.o
if "%1" equ "x" p
@echo on
