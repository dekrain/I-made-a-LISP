# Overview
This is my implementation of @kanaka's Make-A-Lisp, written in C++.
I've modified the specification of the language by my own rules.
Overview of the language below

# Compilation
I've included a `compile.py` script, which you can use to compile the main binary.
It works under Windows, GCC, C++17.
Tested under python 3.
Usage is as follows:
```sh
# Compiles all files
python compile.py

# Compiles only a specific file (for quick code changes)
python compile.py out/obj_file.o

# Only links the binary
python compile.py mal_repl.exe
```

# Language
see: language.md

# Example scripts
see: `scr_fib.mal`
see: `lib/`