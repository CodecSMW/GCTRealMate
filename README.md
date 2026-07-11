## GCTRealMate
A Gecko codeset assembler for Nintendo Wii titles, used primarily within the Super Smash Bros. Brawl community.

## Usage
To assemble a `.asm` file or `.txt` codeset into a `.GCT` file, either drag it onto the program executable, or pass its file path to the program as a command line argument.

### Command Line Arguments
Additionally, GCTRM accepts a handful of extra arguments to affect its output:
| Arg  | Description                                                                                                                           |
|-----:|---------------------------------------------------------------------------------------------------------------------------------------|
| `-l` | Creates a log listing the names of every code assembled into the generated `.GCT`.                                                    |
| `-q` | Skips the `Press enter to close.` prompt after program execution.                                                                     |
| `-t` | Produces a `codeset.txt` file, including each assembled code and its raw hex contents.                                                |
| `-*` | Inserts asterisks in front of each line of raw hex in the generated `codeset.txt` file.                                               |
| `-g` | Prints "RSBE01" at the top of the codeset.txt file, in order to provide a codeset that GCTconvert can use.                            |
| `-r` | Enables attempts to resolve incorrect capitalization in `.include` filepath arguments (Linux only).                                   |
| `-b` | Reads in the following arg as the base address of the GCT. Required for short-branches (eg. `bl $80001234`) in `HOOK` codes.          |
| `-a` | Enables converting absolute branch instructions (eg. `bla`, `ba`) into their relative counterparts. Requires supplying `-b` argument! |
| `-i` | Ignores any arguments for the active codeset provided by the `.ini` file (if one exists). Does nothing when used within the .ini file.|

### Settings File (`.ini`)
GCTRM allows the specification of default arguments for use with any given codeset file through the use of a settings `.ini` file. This file should use the same filename as the program's executable, but with the `.ini` extension (eg. `GCTRealMate.exe` will look for the settings file `GCTRealMate.ini`, `GCTRM.exe` will look for `GCTRM.ini`, etc). Specify entries within the file like so:
> RSBE01.txt : -a -b 0x80566528

Here, `RSBE01.txt` is the name of the file for which the specified arguments should apply, and the arguments following the colon are the arguments themselves. Multiple entries may be specified, like below:

> RSBE01.txt : -a -b 0x80566528  
> BOOST.txt : -a -b 0x80550010  
> NETPLAY.txt : -a -b 0x80566528  
> NETBOOST.txt : -a -b 0x80550010  

This file, when it exists, will be checked for arguments whenever the program is invoked *unless* the `-i` argument is specified on the command line!
