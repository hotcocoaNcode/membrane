begin data
helloworld ascii "hello world!\n\x00"
helloworld_len i8 14
retval i8 0
end

begin symtable
kernel_println
kernel_ret
end

begin exec
li 0 helloworld
li 1 helloworld_len
kcall kernel_println
li 0 retval
kcall kernel_ret
end