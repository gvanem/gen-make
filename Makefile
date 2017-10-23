#
# GNU makefile for gen-make.
# Needs MinGW, MinGW64 or clang-cl.
#

USE_CLANG_CL ?= 0

ifeq ($(USE_CLANG_CL),1)
  CL=
  export CL

  CC     = clang-cl
  CFLAGS = -nologo -MD -Wall -O2 -D_CRT_SECURE_NO_WARNINGS \
           -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE \
           -D_CRT_OBSOLETE_NO_WARNINGS
  RCFLAGS = -nologo -D__clang__

  link_EXE = link -nologo -debug -incremental:no -out:$(strip $(1)) $(2)
  O        = obj
else
  CC       = gcc
  CFLAGS   = -m32 -Wall -O2 # -ggdb
  RCFLAGS = -O COFF --target=pe-i386 # -D__MINGW32__
  link_EXE = $(CC) -m32 -s -o $(1) $(2)
  O        = o
endif

SOURCES = gen-make.c        \
          file_tree_walk.c  \
          smartlist.c       \
          template-mingw.c  \
          template-cygwin.c \
          template-msvc.c   \
          template-watcom.c \
          template-windows.c

ifneq ($(CC),gcc)
  SOURCES += msvc/getopt_long.c
  CFLAGS  += -I.
endif

OBJECTS = $(SOURCES:.c=.$(O))

all: gen-make.exe file_tree_walk.exe

gen-make.exe: $(OBJECTS) gen-make.res
	$(call link_EXE, $@, $^)
	@echo

file_tree_walk.exe: file_tree_walk.c
	$(CC) -c -DTEST $(CFLAGS) $<
	$(call link_EXE, $@, file_tree_walk.$(O))
	rm -f file_tree_walk.$(O)
	@echo

test: gen-make.exe
	cd hello_world                                ; \
	../gen-make -nG windows > Makefile.Windows    ; \
	$(MAKE) -f Makefile.Windows CC=gcc depend all ; \
	foo.exe

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo

%.obj: %.c
	$(CC) $(CFLAGS) -c -Fo./$@ $<
	@echo

ifeq ($(CC),gcc)
gen-make.res: gen-make.rc
	windres $(RCFLAGS) -o $@ $<
	@echo
else
gen-make.res: gen-make.rc
	rc $(RCFLAGS) -fo$@ $<
	@echo
endif

clean:
	rm -f $(OBJECTS) gen-make.exe gen-make.res file_tree_walk.exe
ifneq ($(CC),gcc)
	rm -f gen-make.pdb file_tree_walk.pdb
endif

gen-make.$(O):         gen-make.c gen-make.h
gen-make.res:          gen-make.rc gen-make.h
file_tree_walk.$(O):   file_tree_walk.c gen-make.h
template-mingw.$(O):   template-mingw.c gen-make.h
template-cygwin.$(O):  template-cygwin.c gen-make.h
template-msvc.$(O):    template-msvc.c gen-make.h
template-watcom.$(O):  template-watcom.c gen-make.h
template-windows.$(O): template-windows.c gen-make.h
msvc/getopt_long.$(O): msvc/getopt_long.c msvc/getopt_long.h

