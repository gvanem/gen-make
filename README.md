## gen-make 1.1.0

[![Build Status](https://ci.appveyor.com/api/projects/status/github/gvanem/gen-make?branch=master&svg=true)](https://ci.appveyor.com/project/gvanem/gen-make)

A simple GNU-makefile generator that can generate a makefile for MSVC or clang-cl.
It prints the result to `stdout`.

It works by finding all source-files (`.c`, `*.cc`, `*.cxx` and `*.cpp`) in
current directory and all sub-directories (except `.git`) <br>
and writes the Makefile based on this information. The Makefile is just a
starting point for further hand-editing. Hints are inserted into Makefiles
as `#! xx`.

It also adds:
 * a rule to create a `foo.rc` file.
 * a rule to create dependencies from `SOURCES`.

A generated makefile in this git checkout directory is able to compile and link a program `bin/foo.exe`. <br>
When running `bin/foo.exe`, it should print:<br>

`It seems a "bin/gen-make.exe" generated Makefile was able to compile and build this 'bin/foo.exe' program.` <br>
`Congratulations! But I will not let you do any damage here.`

A future version could use the Python package **[Mako](https://www.makotemplates.org/)**
to generate the templates. Maybe at run-time?
