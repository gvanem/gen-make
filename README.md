## gen-make 1.0.2

[![Build Status](https://ci.appveyor.com/api/projects/status/github/gvanem/gen-make?branch=master&svg=true)](https://ci.appveyor.com/project/gvanem/gen-make)

A simple makefile generator that can generate makefiles for:
 * GNU-make targeting MinGW, clang-cl or MSVC.
 * Microsoft's Nmake targeting MSVC only.
 * OpenWatcom targeting only OpenWatcom programs for Windows.

It supports these generators (option `-G`) and makefile syntaxes:

| *Generator* | *Syntax* |
| :----------| :--------------------|
| `windows` | **GNU-make**, targeting MinGW, MSVC and clang-cl.            |
|           | Use e.g. `make -f Makefile.Windows CC=gcc`.                  |
| `cygwin`  | **GNU-make**                                                 |
| `mingw`   | **GNU-make**, assumes a dual-target MinGW supporting `-m32`. |
| `msvc`    | **Nmake** for Microsoft's make program.                      |
| `watcom`  | **Wmake** for OpenWatcom's make program.                     |

It works by finding all source-files (`.c`, `*.cc`, `*.cxx` and `*.cpp`) in
current directory and all sub-directories (except `.git`) <br>
and writes the Makefile based on this information. The Makefile is just a
starting point for further hand-editing. Hints are inserted into Makefiles
as `#! xx`.

For a GNU-make generator, it also adds:
 * a rule to create a `foo.rc` file.
 * a rule to create dependencies from `SOURCES`.

For the MinGW or Cygwin targets (in generators `window`, `mingw` and `cygwin`),
you can select `x64` target by e.g. a: <br>
  `c:\MyProject> set CPU=x64 & gen-make -G mingw`

This will add a `-m64` to the `CFLAGS` and `LDFLAGS`. It preferably should do this
at the time invoking `Makefile.MinGW` <br>
(not at the time of generating it). You'll have to hand-edit the makefile to fix this.

A generated makefile in this git checkout directory is able to compile and link a program `foo.exe`. <br>
When running `foo.exe`, it should print:<br>

`It seems a "gen-make.exe" generated Makefile was able to compile and build this 'foo.exe' program.` <br>
`Congratulations! But I will not let you do any damage here.`

A future version could use the Python package **[Mako](https://www.makotemplates.org/)**
to generate the templates. Maybe at run-time?
