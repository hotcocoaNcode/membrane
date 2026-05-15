begin data
iterations_max u32 10
output u32 0
end

begin symtable
kernel_ret
end

; 0: result, 1: add1 2: add2, 3: iter, 4: constant 1
begin exec

li 0 0
li 1 1
li 2 0
lwa 3 iterations_max
li 4 1
sub 3 3 4 ; iterations minus 1 because we go to 0
add 0 1 2 ; r0 = r1 + r2
mov 2 1 ; r2 = r1
mov 1 0 ; r1 = r0
sub 3 3 4 ; i--
jmpnz 3 -5 ; i != 0, jump to add
swa 0 output ; store to memory, kernel_ret takes an address
li 0 output ; load output address
kcall kernel_ret
end