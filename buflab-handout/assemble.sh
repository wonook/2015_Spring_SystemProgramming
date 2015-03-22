gcc -c -m32 level4assembly.s
objdump -d level4assembly.o > level4assembly.d
cat level4assembly.d
