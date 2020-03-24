## gen-make 1.0.2

A simple makefile generator that can generate makefiles for:
 * GNU-make targeting MinGW, clang-cl or MSVC.
 * Microsoft's Nmake targeting MSVC only.
 * OpenWatcom targeting only OpenWatcom programs for Windows.

It supports these generators (option `-G`) and makefile syntaxes:

| *Generator* | *Syntax* |
| :----------| :--------------------|
| `windows` | **GNU-make**, targeting MinGW, MSVC and clang-cl. |
|| Use e.g. `make -f Makefile.Windows CC=gcc`. |
| `cygwin`  | **GNU-make** |
| `mingw`   | **GNU-make**, assumes a dual-target MinGW supporting `-m32` |
| `msvc`    | **Nmake** for Microsoft's make program. |
| `watcom`  | **Wmake** for OpenWatcom's make program. |

It works by finding all source-files (`.c`, `*.cc`, `*.cxx` and `*.cpp`) in
current directory and all sub-directories (except `.git`) <br>
and writes the Makefile based on this information. The Makefile is just a
starting point for further hand-editing. Hints are inserted into Makefiles
at `#! xx`.

For a GNU-make generator, it also adds:
 * a rule to create a `foo.rc` file.
 * a rule to create dependencies from `SOURCES`.

For the MinGW or Cygwin targets (generators `window`, `mingw` and `cygwin`),
you can select CPU x64 target by `set CPU=x64 & gen-make -G mingw`. This will
add a `-m64` to the `CFLAGS` and `LDFLAGS`.

A generated makefile in this git checkout directory is able to compile and
link a program `foo.exe`.
