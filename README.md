# Assembler_for_an_Abstract_machine

compile:
g++ -std=c++17 -O2 -Wall -Wextra -o assembler main.cpp worklet2.cpp

run:
./assembler t2.asm out.bin

validate:
objdump -D -b binary -mi386 -M intel out.bin

