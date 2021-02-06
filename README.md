# What is the Dallas VM?
The DAVM is a virtual machine that interprets an assembly language that's a cross between Z80 and ARM assembly.
# Hello, world!
```
STR R0 "Hello, world!\n"
REV #1
RET 0
```
STR stores an address to a string in a register (R0, register 0) and string "Hello, world!" plus a newline.

REV runs a command, in this case print string.

RET 0 exits.

# All values
| Type       | Syntax     | Description     |
| :------------- | :----------: | -----------: |
|  Register | R(num) | Gets a register |
|  Number | #(num) | Returns a number |
|  Location | (num) | Gets the value in the memory |
|  String | "(text)" | Returns the text |
# All mnemonics
> Note: value is any non string value.

| Mnemonic       | Syntax     | Description     |
| :------------- | :----------: | -----------: |
|  LD |  LD register value | Loads a non-str value into a register    |
|  STR | STR register string | Loads the address of a string into a register |
|  REV | REV number | Runs a command. Currently supports 1 (print string), 2 (print char), 3 (print number), 4 (store a char in R0), 5 (get a string and store it in R0), and 6 (clear screen) |
|  RET | RET number(optional) | Gets a character, ends the window, and exits with the number passed (defaults to 0) |
|  SET | SET location value | Sets the location as value |
|  Math mnemonics | (mnemonic) register value value | Performs operation on value and value and puts it in register. This includes ADD, SUB, MUL, DIV, POW, AND, OR, XOR, and NOT. |
|  NOT | NOT register value | Performs not operation on value and stores in register |