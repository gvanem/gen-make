#ifndef _GEN_MAKE_H
#define _GEN_MAKE_H

#define VER_MAJOR 1
#define VER_MINOR 0
#define VER_MICRO 2

#if !defined(RC_INVOKED)  /* Rest of file */

#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <io.h>
#include <windows.h>

#define DIM(array)        (sizeof(array) / sizeof(array[0]))
#define FILE_EXISTS(file) (access(file,0) == 0)

#if defined(TEMPLATE_IS_CYGWIN)
  #define TEMPLATE_IS_GNUMAKE
  #define TEMPLATE_MAKE  "GNU"
  #define TEMPLATE_NAME  "CygWin"
  #define THIS_FILE      "Makefile.CygWin"

#elif defined(TEMPLATE_IS_MINGW)
  #define TEMPLATE_IS_GNUMAKE
  #define TEMPLATE_MAKE  "GNU"
  #define TEMPLATE_NAME  "MinGW"
  #define THIS_FILE      "Makefile.MinGW"

#elif defined(TEMPLATE_IS_WATCOM)
  #define TEMPLATE_MAKE  "Watcom"
  #define TEMPLATE_NAME  "Watcom"
  #define THIS_FILE      "$(__MAKEFILES__)"

#elif defined(TEMPLATE_IS_MSVC)
  #define TEMPLATE_MAKE  "Nmake"
  #define TEMPLATE_NAME  "MSVC"
  #define THIS_FILE      "Makefile.MSVC"

#elif defined(TEMPLATE_IS_WINDOWS)
  #define TEMPLATE_IS_GNUMAKE
  #define TEMPLATE_MAKE  "GNU"
  #define TEMPLATE_NAME  "Windows"
  #define THIS_FILE      "Makefile.Windows"

#elif defined(TEMPLATE_IS_NONE)
  /*
   * Compiling gen-make.c
   */
#else
  #error 'TEMPLATE_x must be defined.'
#endif

#define TEMPLATE_TOP                                                       \
        "#",                                                               \
        "# " TEMPLATE_MAKE " Makefile for project X / " TEMPLATE_NAME ".", \
        "# Generated by gen-make at %D.",                                  \
        "#",                                                               \
        "THIS_FILE = " THIS_FILE,                                          \
        "",                                                                \
        "VER_MAJOR = 1  #! Change this",                                   \
        "VER_MINOR = 2  #! Change this",                                   \
        "VER_PATCH = 3  #! Change this",                                   \
        "VERSION   = $(VER_MAJOR).$(VER_MINOR).$(VER_PATCH)",              \
        TEMPLATE_DATE,                                                     \
        TEMPLATE_GREEN_MSG,                                                \
        "",                                                                \
        "%v",                                                              \
        "",                                                                \
        "#",                                                               \
        "# Choose your weapons.",                                          \
        "#",                                                               \
        "USE_OPENSSL   = 0",                                               \
        "USE_CRT_DEBUG = 0",                                               \
        "",                                                                \
        "PROGRAM = foo.exe #! Change this",                                \
        "",                                                                \
        "#",                                                               \
        "# Location of required packages",                                 \
        "#",                                                               \
        "OPENSSL_ROOT = " TEMPLATE_OPENSSL_ROOT

#define TEMPLATE_GMAKE_MACROS                                                                                        \
        "#",                                                                                                         \
        "# GNU-make macros:",                                                                                        \
        "# (not all may be needed)",                                                                                 \
        "#",                                                                                                         \
        "define link_EXE",                                                                                           \
        "  $(call green_msg, Linking $(1))",                                                                         \
        "  $(call link_EXE_$(CC), $(1), $(2))",                                                                      \
        "  @echo",                                                                                                   \
        "endef",                                                                                                     \
        "",                                                                                                          \
        "define link_EXE_cl",                                                                                        \
        "  link $(LDFLAGS) -out:$(strip $(1)) $(2) > link.tmp",                                                      \
        "  @cat link.tmp >> $(1:.exe=.map)",                                                                         \
        "  @rm -f link.tmp",                                                                                         \
        "endef",                                                                                                     \
        "",                                                                                                          \
        "link_EXE_gcc      = $(CC) $(LDFLAGS) -o $(1) $(2) > $(1:.exe=.map)",                                        \
        "link_EXE_clang-cl = $(call link_EXE_cl, $(1), $(2))",                                                       \
        "",                                                                                                          \
        "#",                                                                                                         \
        "# A DLL link macro:",                                                                                       \
        "#  arg1, $(1): The DLL-file to create.",                                                                    \
        "#  arg2, $(2): The import library.",                                                                        \
        "#  arg3, $(3): The rest of the arguments.",                                                                 \
        "#",                                                                                                         \
        "define link_DLL",                                                                                           \
        "  $(call green_msg, Linking $(1))",                                                                         \
        "  $(call link_DLL_$(CC), $(1), $(2), $(3))",                                                                \
        "  @echo",                                                                                                   \
        "endef",                                                                                                     \
        "",                                                                                                          \
        "define link_DLL_cl",                                                                                        \
        "  link $(LDFLAGS) -dll -out:$(strip $(1)) -implib:$(strip $(2)) $(3) > link.tmp",                           \
        "  @cat link.tmp >> $(1:.dll=.map)",                                                                         \
        "  @rm -f link.tmp $(2:.lib=.exp)",                                                                          \
        "endef",                                                                                                     \
        "",                                                                                                          \
        "link_DLL_gcc      = $(CC) -shared $(LDFLAGS) -o $(1) -Wl,--out-implib,$(strip $(2)) $(3) > $(1:.dll=.map)", \
        "link_DLL_clang-cl = $(call link_DLL_cl, $(1), $(2), $(3)",                                                  \
        "",                                                                                                          \
        "define make_lib",                                                                                           \
        "  $(call green_msg, Creating $(1))",                                                                        \
        "  rm -f $(1)",                                                                                              \
        "  $(call make_lib_$(CC), $(1), $(2))",                                                                      \
        "  @echo",                                                                                                   \
        "endef",                                                                                                     \
        "",                                                                                                          \
        "make_lib_gcc = ar rs $(1) $(2)",                                                                            \
        "make_lib_cl  = lib -nologo -machine:x86 -out:$(strip $(1)) $(2)",                                           \
        "make_lib_clang-cl = $(call make_lib_cl, $(1), $(2))",                                                       \
        "",                                                                                                          \
        "define make_res",                                                                                           \
        "  $(call green_msg, Creating $(1))",                                                                        \
        "  $(call make_res_$(CC), $(1), $(2))",                                                                      \
        "  @echo",                                                                                                   \
        "endef",                                                                                                     \
        "",                                                                                                          \
        "make_res_gcc      = windres -D__MINGW32__ --target=pe-i386 -O COFF -o $(2) $(1)",                           \
        "make_res_cl       = rc -D_MSC_VER -nologo -Fo./$(strip $(2)) $(1)",                                         \
        "make_res_clang-cl = rc -D__clang__ -nologo -Fo./$(strip $(2)) $(1)",                                        \
        ""

#if defined(TEMPLATE_IS_WINDOWS)
  #define DEPEND_GCC "\tgcc"
#else
  #define DEPEND_GCC "\t$(CC)"
#endif

#if defined(TEMPLATE_IS_GNUMAKE)
  #define TEMPLATE_DEPEND                                                                                      \
          "DEP_REPLACE = sed -e 's/\\(.*\\)\\.o: /\\n$$(OBJ_DIR)\\/\\1.$$(O): /'",                             \
          "",                                                                                                  \
          "depend: $(GENERATED)",                                                                              \
          DEPEND_GCC " -MM $(filter -I% -D%, $(CFLAGS)) $(SOURCES) | $(DEP_REPLACE) > .depend." TEMPLATE_NAME, \
          "",                                                                                                  \
          "-include .depend." TEMPLATE_NAME

  #define TEMPLATE_CONFIG                                                                         \
          "define WARNING",                                                                       \
          "  /*",                                                                                 \
          "   * DO NOT EDIT! This file was automatically generated",                              \
          "   * from $(realpath $(THIS_FILE)) at $(DATE). Edit that file instead.",               \
          "   */",                                                                                \
          "endef",                                                                                \
          "",                                                                                     \
          "define CONFIG_H",                                                                      \
          "  #define WIN32_LEAN_AND_MEAN",                                                        \
          "  #if defined(_MSC_VER)",                                                              \
          "    #ifndef _CRT_NONSTDC_NO_WARNINGS",                                                 \
          "    #define _CRT_NONSTDC_NO_WARNINGS",                                                 \
          "    #endif",                                                                           \
          "    #ifndef _CRT_OBSOLETE_NO_WARNINGS",                                                \
          "    #define _CRT_OBSOLETE_NO_WARNINGS",                                                \
          "    #endif",                                                                           \
          "    #ifndef _CRT_SECURE_NO_DEPRECATE",                                                 \
          "    #define _CRT_SECURE_NO_DEPRECATE",                                                 \
          "    #endif",                                                                           \
          "    #ifndef _CRT_SECURE_NO_WARNINGS",                                                  \
          "    #define _CRT_SECURE_NO_WARNINGS",                                                  \
          "    #endif",                                                                           \
          "    /* !Add more stuff here... */",                                                    \
          "  #endif",                                                                             \
          "",                                                                                     \
          "  #include <stdlib.h>",                                                                \
          "  /* !Add more stuff here */",                                                         \
          "endef",                                                                                \
          "",                                                                                     \
          "config.h: $(THIS_FILE)",                                                               \
          "\t$(call green_msg, Generating $@)",                                                   \
          "\t$(file >  $@,$(WARNING))",                                                           \
          "\t$(file >> $@,#ifndef _CONFIG_H)",                                                    \
          "\t$(file >> $@,#define _CONFIG_H)",                                                    \
          "\t$(file >> $@,$(CONFIG_H))",                                                          \
          "\t$(file >> $@,#endif /* _CONFIG_H */)"

#elif defined(TEMPLATE_IS_MSVC)
  #define TEMPLATE_CONFIG                                                                         \
          "config.h: $(THIS_FILE)",                                                               \
          "\t@echo /* config.h for " TEMPLATE_NAME ". DO NOT EDIT! > $@",                         \
          "\t@echo  */                                            >> $@",                         \
          "\t@echo #ifndef _CRT_NONSTDC_NO_WARNINGS  >> $@",                                      \
          "\t@echo #define _CRT_NONSTDC_NO_WARNINGS  >> $@",                                      \
          "\t@echo #endif                            >> $@",                                      \
          "\t@echo #ifndef _CRT_OBSOLETE_NO_WARNINGS >> $@",                                      \
          "\t@echo #define _CRT_OBSOLETE_NO_WARNINGS >> $@",                                      \
          "\t@echo #endif                            >> $@",                                      \
          "\t@echo #ifndef _CRT_SECURE_NO_DEPRECATE  >> $@",                                      \
          "\t@echo #define _CRT_SECURE_NO_DEPRECATE  >> $@",                                      \
          "\t@echo #endif                            >> $@",                                      \
          "\t@echo #ifndef _CRT_SECURE_NO_WARNINGS   >> $@",                                      \
          "\t@echo #define _CRT_SECURE_NO_WARNINGS   >> $@",                                      \
          "\t@echo #endif                            >> $@",                                      \
          "\t@echo /* !Add more stuff here...*/      >> $@",                                      \
          ""
#else
  #define TEMPLATE_CONFIG                                                                         \
          "config.h: $(THIS_FILE)",                                                               \
          "\t@echo /* config.h for " TEMPLATE_NAME ". DO NOT EDIT! */ > $@",                      \
          "\t@echo /* !Add more stuff here */             >> $@",                                 \
          ""
#endif

#if defined(TEMPLATE_IS_GNUMAKE)
  #define TEMPLATE_DATE        "",                              \
                               "DATE = $(shell date +%d-%B-%Y)" \
                               ""

  #define TEMPLATE_GREEN_MSG   "",                                                                  \
                               "#",                                                                 \
                               "# This assumes you have CygWin/Msys's 'echo' with colour support.", \
                               "#",                                                                 \
                               "green_msg = @echo -e '\\e[1;32m$(strip $(1))\\e[0m'"
#else
  #define TEMPLATE_DATE        ""
  #define TEMPLATE_GREEN_MSG   ""
#endif

#define TEMPLATE_GCC_CFLAGS             "-m%b -Wall"
#define TEMPLATE_GCC_LDFLAGS            "-m%b -Wl,--print-map,--sort-common"

#define TEMPLATE_CL_CFLAGS              "-nologo -W3 -Zi -O2 -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS \\\n"              \
                                        "             -D_CRT_OBSOLETE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE \\\n" \
                                        "             -D_CRT_SECURE_NO_WARNINGS"
#define TEMPLATE_CL_LDFLAGS             "-nologo -debug -incremental:no -verbose"

#define TEMPLATE_COMMON_CFLAGS          "-D_WIN32_WINNT=0x0501 -DHAVE_CONFIG_H -I. #! Add include dirs as needed"

#if defined(TEMPLATE_IS_CYGWIN)
  #define TEMPLATE_OPENSSL_ROOT         "#! Not needed. OpenSSL should be installed under /usr/include"

#elif defined(TEMPLATE_IS_GNUMAKE)
  #define TEMPLATE_OPENSSL_ROOT         "$(MINGW_ROOT)/src/inet/Crypto/OpenSSL  #! Example requirement."

#else  /* MSVC/Watcom */
  #define TEMPLATE_OPENSSL_ROOT         "f:\\net\\Crypto\\OpenSSL   #! Example requirement."
#endif

#if defined(TEMPLATE_IS_CYGWIN)
  #define TEMPLATE_OPENSSL_CFLAGS       "-DHAVE_OPENSSL -DOPENSSL_USE_DEPRECATED"
  #define TEMPLATE_OPENSSL_GCC_EX_LIBS  "/usr/lib/libssl.a /usr/lib/libcrypto.a"
#else
  #define TEMPLATE_OPENSSL_CFLAGS       "-DHAVE_OPENSSL -DOPENSSL_USE_DEPRECATED -I$(OPENSSL_ROOT)/include"
  #define TEMPLATE_OPENSSL_GCC_EX_LIBS  "$(OPENSSL_ROOT)/libssl.dll.a $(OPENSSL_ROOT)/libcrypto.dll.a"
  #define TEMPLATE_OPENSSL_EX_LIBS      "$(OPENSSL_ROOT)/libssl_imp.lib $(OPENSSL_ROOT)/libcrypto_imp.lib"
#endif

#define TEMPLATE_GCC_EX_LIBS            "-lws2_32    #! Add more libs as needed"
#define TEMPLATE_EX_LIBS                "ws2_32.lib  #! Add more libs as needed"

extern const char *cygwin_makefile_name, *mingw_makefile_name, *msvc_makefile_name;
extern const char *watcom_makefile_name, *windows_makefile_name;

extern const char *cygwin_c_rule,  *cygwin_cc_rule,  *cygwin_cpp_rule,  *cygwin_cxx_rule,  *cygwin_res_rule,  *cygwin_lib_rule;
extern const char *mingw_c_rule,   *mingw_cc_rule,   *mingw_cpp_rule,   *mingw_cxx_rule,   *mingw_res_rule,   *mingw_lib_rule;
extern const char *msvc_c_rule,    *msvc_cc_rule,    *msvc_cpp_rule,    *msvc_cxx_rule,    *msvc_res_rule,    *msvc_lib_rule;
extern const char *watcom_c_rule,  *watcom_cc_rule,  *watcom_cpp_rule,  *watcom_cxx_rule,  *watcom_res_rule,  *watcom_lib_rule;
extern const char *windows_c_rule, *windows_cc_rule, *windows_cpp_rule, *windows_cxx_rule, *windows_res_rule, *windows_lib_rule;

extern int generate_cygwin_make (FILE *out);
extern int generate_mingw_make (FILE *out);
extern int generate_msvc_nmake (FILE *out);
extern int generate_watcom_wmake (FILE *out);
extern int generate_windows_make (FILE *out);

extern void Abort (const char *fmt, ...);
extern int  write_template_line (FILE *out, const char *str);

typedef int (*walker_func) (const char *found, const WIN32_FIND_DATA *ff_data);

extern DWORD file_tree_walk (const char *dir, walker_func func);

#endif  /* RC_INVOKED */
#endif  /* _GEN_MAKE_H */
