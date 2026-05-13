begin data
helloworld ascii "hello world!\n\x00"
helloworld_len i8 14
retval i8 1
end

begin symtable
kernel_println
kernel_ret
end

begin exec
li 0 helloworld
li 1 helloworld_len
kcall 0
li 0 retval
kcall 1
end