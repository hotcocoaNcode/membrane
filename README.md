# membrane
> *bytecode interpreter and assembler*

## Overview

Membrane is a low-overhead reduced instruction set bytecode (RISB?) intended for use on 32-bit microcontrollers and constrained platforms running Coenosarc (a single-tasking microkernel I am working on). Its portability is inspired by Java and my previous (albeit very very bad) language and bytecode, Fluff. 

This program implements an assembler for its bytecode, and an emulator for running said bytecode. Eventually this emulator may support standard Coenosarc system calls, however I have no idea what those will look like yet, and have left some temporary utility calls in their place.

To assemble a `.ms` (membrane source) file, simply execute `membrane ./path/to/file.ms`. To execute a `.mbn` (membrane) file, execute `membrane ./path/to/executable.mbn`.

## Assembly Language

There are three sections to an executable, `data`, `symtable`, and `exec`.
Each section is started with a `begin section_type_here` statement and finished with a `end` statement.
It is encouraged that you only have one of each, and creating more than one of each is unsupported, however based on the current codebase I have no reason to believe that shouldn't work. 

Examples can be found in `./asm_examples/` if that ends up being of help to you.

### Executable Data

The data section's syntax is quite simple, and will feel familiar to anyone who has worked with modern assembly before but is most definitely not entirely the same. One important reminder is that the memory executable data is stored in, on this platform, is not necessarily read-only as it may be on others. It *will not persist on-disk* if changed, but may be modified and used over the course of the program.
A line in the data section has three space-separated arguments:

`name type value`

The name can be any string without whitespace, and will be used as an identifier for the data's memory address. 
The type can be any one of the seven supported data types;
- u8
- u16
- u32
- i8
- i16
- i32
- ascii

Types prefixed with u are unsigned integers, types prefixed with i are signed. The seventh type, ascii, is a quote contained ascii string, supporting C-style escape characters such as `\n` and `\r`, as well as custom hex escapes such as `\x00`.

### Symbol Table

The symbol table is written directly to the executable, and tells the interpreter/JIT compiler/AOT compiler/whatever runs this on your platform what functions will be required for this program. 
The only currently supported functions for this interpreter are `kernel_println` and `kernel_ret`, but later I'll probably add input in some format before writing Coenosarc and switching to emulator compatibility rather than my own definitions. 
To register a required function in the symbol table, just write its name (max of 24 characters).
You can then use it in executable code (i.e. `kcall kernel_println`).

### Executed Code

A lot of this is loosely similar to RV32IM syntax, because that was its original JIT target. 
If you are familiar with RV32IM, you will probably recognize most of this, however it does have differences.

There are, at maximum, 256 supported registers as the IDs are 8 bits. However, every target reserves the right to modulo each virtual register ID by some power of 2, being at minimum 8. This means, in edge cases, sane programs may not be supported on non-sane systems. Additionally, in *other* types of edge cases, non-sane programs may not be supported on sane systems. 

There *are* 256 registers. This is solely because I'd like each register ID to be one byte.

*Please do not use 256 registers*.

The entrypoint for all programs is, as of version 1, always the first instruction in the `exec` section. 
It is encouraged you structure your code around this. A table of instructions is below. Any register (just its ID in actual syntax) from 0-256 is to be substituted for any given `ra`, `rb`, or `rc`.
Anywhere it says 'address', any name from the data section or symbol table should be permitted in its place as well.

| Instruction | Arguments | Notes |
|---|---|---|
| nop | none | does nothing. |
| li | dst ra, 16 bit constant | set the lower 16 bits of given register `ra` to a 16 bit constant. |
| lui | dst ra, 16 bit constant | set the upper 16 bits of given register `ra` to a 16 bit constant. |
| lwa | dst ra, 16 bit address | load a 32 bit word to `ra` from a constant memory address |
| lvr | dst ra, src rb, 8 bit constant N | load an N-byte chunk (values 1, 2, and 4 are defined behavior) to `ra` from a memory address stored in `rb` |
| lwr | dst ra, src rb | load a 32 bit word to `ra` from a memory address stored in `rb` |
| swa | dst ra, 16 bit address | stores a 32 bit word from `ra` to a constant memory address |
| svr | dst ra, adr rb, 8 bit constant N | stores an N-byte chunk (values 1, 2, and 4 are defined behavior) to a memory address stored in `rb`, from `ra` |
| swr | dst ra, adr rb | load a 32 bit word from `ra` to a memory address stored in `rb` |
| mov | dst ra, src rb | copy the value from `rb` into `ra` |
| add | dst ra, arg rb, arg rc | `ra = rb + rc` |
| sub | dst ra, arg rb, arg rc | `ra = rb - rc` |
| mul | dst ra, arg rb, arg rc | `ra = rb * rc` |
| div | dst ra, arg rb, arg rc | `ra = rb / rc` |
| mod | dst ra, arg rb, arg rc | `ra = rb % rc` |
| and | dst ra, arg rb, arg rc | `ra = rb & rc` |
| or | dst ra, arg rb, arg rc | `ra = rb \| rc` |
| xor | dst ra, arg rb, arg rc | `ra = rb ^ rc` |
| not | dst ra, src rb | `ra = ~rb` |
| shl | dst ra, arg rb, arg rc | `ra = rb << rc` |
| shr | dst ra, arg rb, arg rc | `ra = rb >> rc` |
| eq | dst ra, arg rb, arg rc | `ra = 1` if `rb == rc`, `ra = 0` if not |
| neq | dst ra, arg rb, arg rc | `ra = 1` if `rb != rc`, `ra = 0` if not |
| ltu | dst ra, arg rb, arg rc | `ra = 1` if `rb < rc`, `ra = 0` if not |
| lts | dst ra, arg rb, arg rc | `ra = 1` if `rb < rc`, `ra = 0` if not |
| lteu | dst ra, arg rb, arg rc | `ra = 1` if `rb <= rc`, `ra = 0` if not |
| ltes | dst ra, arg rb, arg rc | `ra = 1` if `rb <= rc`, `ra = 0` if not |
| jmp | 16 bit signed constant | relative to the next instruction, add signed constant to program counter |
| jmpz | arg ra, 16 bit signed constant | if `ra == 0`, relative to the next instruction, add signed constant to program counter |
| jmpnz | arg ra, 16 bit signed constant | if `ra != 0`, relative to the next instruction, add signed constant to program counter |
| mark | name | syntactic sugar. defines a `call`-able name for the next instruction in the program. |
| call | 24 bit address | set program counter to a given value, and push the address of the next instruction to the callstack. |
| kcall | 24 bit address | drop out of program and call a system function from the symbol table, defined by runtime implementation. |
| ret | none | pop the callstack, program counter returns to pre-call function |
| halt | none | stop the program entirely |

## Bytecode

Bytecode has 32 bit instructions, in format `[opcode, data byte 1, data byte 2, data byte 3]`. All data is LE, therefore a 24 bit integer `0xABCDEF` would be stored as `[opcode, EF, CD, AB]`. This means, on little endian systems (pretty much every target this is intended for) a `struct` that describes an instruction should be formatted as such (this is pseudocode);
```
struct inst {
    u8 data[3]
    u8 opcode
}
```

If you somehow end up working with the bytecode itself, I assume you can read code and will end up browsing this project anyways. I'm not going to list every instruction here, because that would be a lot more typing than I'd like. Please see src/mbnwrite.cpp or src/mbnemu.cpp for reference.