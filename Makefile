#
# GNU makefile for gen-make.
# Needs MinGW, MinGW64 or clang-cl.
#

USE_CLANG_CL ?= 1
OBJ_DIR = objects

ifeq ($(USE_CLANG_CL),1)
  export CL=

  CC     = clang-cl
  CFLAGS = -nologo -MD -W2 -O2 -D_CRT_SECURE_NO_WARNINGS \
           -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE \
           -D_CRT_OBSOLETE_NO_WARNINGS
  RCFLAGS = -nologo -D__clang__

  link_EXE = link -nologo -debug -incremental:no -out:$(strip $(1)) $(2)
  O        = obj
else
  CC       = gcc
  CFLAGS   = -m32 -Wall -O2
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
  VPATH    = msvc
  SOURCES += msvc/getopt_long.c
  CFLAGS  += -I.
endif

OBJECTS = $(addprefix $(OBJ_DIR)/, \
            $(notdir $(SOURCES:.c=.$(O))) )

all: $(OBJ_DIR) gen-make.exe file_tree_walk.exe

$(OBJ_DIR):
	- mkdir $@

gen-make.exe: $(OBJECTS) $(OBJ_DIR)/gen-make.res
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

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo

$(OBJ_DIR)/%.obj: %.c
	$(CC) $(CFLAGS) -c -Fo./$@ $<
	@echo

ifeq ($(CC),gcc)
$(OBJ_DIR)/gen-make.res: gen-make.rc
	windres $(RCFLAGS) -o $@ $<
	@echo
else
$(OBJ_DIR)/gen-make.res: gen-make.rc
	rc $(RCFLAGS) -fo$@ $<
	@echo
endif

clean:
	rm -f $(OBJ_DIR)/* gen-make.exe gen-make.res file_tree_walk.exe
	rm -f gen-make.pdb file_tree_walk.pdb

$(OBJ_DIR)/gen-make.$(O):         gen-make.c gen-make.h
$(OBJ_DIR)/gen-make.res:          gen-make.rc gen-make.h
$(OBJ_DIR)/file_tree_walk.$(O):   file_tree_walk.c gen-make.h
$(OBJ_DIR)/template-mingw.$(O):   template-mingw.c gen-make.h
$(OBJ_DIR)/template-cygwin.$(O):  template-cygwin.c gen-make.h
$(OBJ_DIR)/template-msvc.$(O):    template-msvc.c gen-make.h
$(OBJ_DIR)/template-watcom.$(O):  template-watcom.c gen-make.h
$(OBJ_DIR)/template-windows.$(O): template-windows.c gen-make.h
$(OBJ_DIR)/getopt_long.$(O):      msvc/getopt_long.c msvc/getopt_long.h

