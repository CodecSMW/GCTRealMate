## GCTRealMate
A Gecko codeset assembler for Nintendo Wii titles, used primarily within the Super Smash Bros. Brawl community.

## Usage
To assemble a `.asm` file or `.txt` codeset into a `.GCT` file, either drag it onto the program executable, or pass its file path to the program as a command line argument.

Additionally, GCTRM accepts a handful of extra arguments to affect its output:
| Arg | Description                                                                                                 |
|----:|-------------------------------------------------------------------------------------------------------------|
| "-l" | Creates a log listing the names of every code assembled into the generated `.GCT`.                         |
| "-q" | Skips the `Press enter to close.` prompt after program execution.                                          |
| "-t" | Produces a `codeset.txt` file, including each assembled code and its raw hex contents.                     |
| "-*" | Inserts asterisks in front of each line of raw hex in the generated `codeset.txt` file.                    |
| "-g" | Prints "RSBE01" at the top of the codeset.txt file, in order to provide a codeset that GCTconvert can use. |
| "-r" | Enables attempts to resolve incorrect capitalization in `.include` filepath arguments (Linux only).        |
