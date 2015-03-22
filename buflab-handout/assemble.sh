gcc -c -m32 level3assembly.s
objdump -d level3assembly.o > level3assembly.d
cat level3assembly.d
